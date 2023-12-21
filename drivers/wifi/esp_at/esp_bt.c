#include "esp.h"
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/rb.h>
#include <zephyr/sys/mem_blocks.h>

#if CONFIG_WIFI_ESP_BLUETOOTH_SHIM

LOG_MODULE_DECLARE(wifi_esp_at);

#define UNSUPPORTED_ADV_OPTIONS (BT_LE_ADV_OPT_ONE_TIME | \
	BT_LE_ADV_OPT_USE_IDENTITY | BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY | \
	BT_LE_ADV_OPT_DIR_ADDR_RPA | BT_LE_ADV_OPT_FILTER_SCAN_REQ | \
	BT_LE_ADV_OPT_FILTER_CONN | BT_LE_ADV_OPT_NOTIFY_SCAN_REQ | \
	BT_LE_ADV_OPT_ANONYMOUS | BT_LE_ADV_OPT_USE_TX_POWER | \
	BT_LE_ADV_OPT_DISABLE_CHAN_37 | BT_LE_ADV_OPT_DISABLE_CHAN_38 | \
	BT_LE_ADV_OPT_DISABLE_CHAN_39 | BT_LE_ADV_OPT_USE_NRPA | \
	BT_LE_ADV_OPT_FORCE_NAME_IN_AD)

struct gatt_attr_db_ent {
	struct rbnode node;
	struct bt_gatt_attr const *attr;
	int16_t svc_index;
	int16_t chrc_index;
};

static bool gatt_attr_db_cmp(struct rbnode *a, struct rbnode *b);

extern struct esp_data esp_driver_data;

SYS_MEM_BLOCKS_DEFINE_STATIC(gatt_attr_db_pool, sizeof(struct gatt_attr_db_ent), CONFIG_BT_MAX_ATTRS, sizeof(void*));
static struct rbtree gatt_attr_db = {
	.lessthan_fn = gatt_attr_db_cmp,
};

static size_t get_ad_pdu_len(const struct bt_data *ad, size_t ad_len)
{
	size_t result = 0;
	for (size_t i = 0; i < ad_len; ++i) {
		result += ad[i].data_len + 2;
	}
	return result;
}

int bt_enable(bt_ready_cb_t cb)
{
	k_work_cancel(&esp_driver_data.bt_enable_work);
	esp_driver_data.bt_ready_cb = cb;
	int err = k_work_submit_to_queue(&esp_driver_data.workq, &esp_driver_data.bt_enable_work);
	if (err < 0) {
		return err;
	}
	return 0;
}

int bt_le_adv_start(const struct bt_le_adv_param *param,
	const struct bt_data *ad, size_t ad_len,
	const struct bt_data *sd, size_t sd_len)
{
	k_work_cancel(&esp_driver_data.bt_adv_start_work);

	size_t ad_pdu_len = get_ad_pdu_len(ad, ad_len);
	size_t sd_pdu_len = get_ad_pdu_len(sd, sd_len);

	if (ad_pdu_len + sd_pdu_len > sizeof(esp_driver_data.bt_adv_data)) {
		return -EINVAL;
	}

	memcpy(&esp_driver_data.bt_adv_param, param, sizeof(struct bt_le_adv_param));

	esp_driver_data.bt_adv_ad_len = ad_pdu_len;
	esp_driver_data.bt_adv_sd_len = sd_pdu_len;

	size_t cursor = 0;
	for (size_t i = 0; i < ad_len; ++i) {
		esp_driver_data.bt_adv_data[cursor++] = ad[i].data_len + 1;
		esp_driver_data.bt_adv_data[cursor++] = ad[i].type;
		memcpy(&esp_driver_data.bt_adv_data[cursor], ad[i].data, ad[i].data_len);
		cursor += ad[i].data_len;
	}

	for (size_t i = 0; i < sd_len; ++i) {
		esp_driver_data.bt_adv_data[cursor++] = sd[i].data_len + 1;
		esp_driver_data.bt_adv_data[cursor++] = sd[i].type;
		memcpy(&esp_driver_data.bt_adv_data[cursor], sd[i].data, sd[i].data_len);
		cursor += sd[i].data_len;
	}

	int err = k_work_submit_to_queue(&esp_driver_data.workq, &esp_driver_data.bt_adv_start_work);
	if (err < 0) {
		return err;
	}

	k_sem_take(&esp_driver_data.sem_bt_adv_started, K_FOREVER);

	return esp_driver_data.sem_bt_adv_err;
}

int bt_le_adv_stop()
{
	int err = k_work_submit_to_queue(&esp_driver_data.workq, &esp_driver_data.bt_adv_stop_work);
	if (err < 0) {
		return err;
	}
	return 0;
}

int bt_conn_set_security(struct bt_conn *conn, bt_security_t sec_level)
{
	return -ENOTSUP;
}

ssize_t bt_gatt_attr_read(struct bt_conn *conn, const struct bt_gatt_attr *attr,
	void *buf, uint16_t buf_len, uint16_t offset,
	const void *value, uint16_t value_len)
{
	if (offset > value_len) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	uint16_t len = MIN(buf_len, value_len - offset);

	memcpy(buf, (uint8_t*)value + offset, len);

	return len;
}

ssize_t bt_gatt_attr_read_service(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	struct bt_uuid *uuid = attr->user_data;

	if (uuid->type == BT_UUID_TYPE_16) {
		uint16_t uuid16 = sys_cpu_to_le16(BT_UUID_16(uuid)->val);

		return bt_gatt_attr_read(conn, attr, buf, len, offset,
					 &uuid16, 2);
	}

	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 BT_UUID_128(uuid)->val, 16);
}

ssize_t bt_gatt_attr_read_chrc(struct bt_conn *conn,
	const struct bt_gatt_attr *attr, void *buf,
	uint16_t len, uint16_t offset)
{
	return -ENOTSUP;
}

ssize_t bt_gatt_attr_read_ccc(struct bt_conn *conn,
	const struct bt_gatt_attr *attr, void *buf,
	uint16_t len, uint16_t offset)
{
	return -ENOTSUP;
}

ssize_t bt_gatt_attr_write_ccc(struct bt_conn *conn,
	const struct bt_gatt_attr *attr, const void *buf,
	uint16_t len, uint16_t offset, uint8_t flags)
{
	return -ENOTSUP;
}

int bt_gatt_notify_cb(struct bt_conn *conn,
	struct bt_gatt_notify_params *params)
{
	int err = k_is_in_isr()
		? k_sem_take(&conn->bt_notify_busy, K_NO_WAIT)
		: k_sem_take(&conn->bt_notify_busy, K_FOREVER);

	if (err) {
		return err;
	}

	memcpy(&conn->notify_params, params, sizeof(struct bt_gatt_notify_params));
	memcpy(conn->notify_buf, params->data, params->len);
	conn->notify_params.data = conn->notify_buf;

	err = k_work_submit_to_queue(&esp_driver_data.workq, &conn->bt_notify_work);
	if (err < 0) {
		k_sem_give(&conn->bt_notify_busy);
		return err;
	}

	return 0;
}

void bt_uuid_to_str(const struct bt_uuid *uuid, char *str, size_t len)
{
	uint32_t tmp1, tmp5;
	uint16_t tmp0, tmp2, tmp3, tmp4;

	switch (uuid->type) {
	case BT_UUID_TYPE_16:
		snprintk(str, len, "%04x", BT_UUID_16(uuid)->val);
		break;
	case BT_UUID_TYPE_32:
		snprintk(str, len, "%08x", BT_UUID_32(uuid)->val);
		break;
	case BT_UUID_TYPE_128:
		memcpy(&tmp0, &BT_UUID_128(uuid)->val[0], sizeof(tmp0));
		memcpy(&tmp1, &BT_UUID_128(uuid)->val[2], sizeof(tmp1));
		memcpy(&tmp2, &BT_UUID_128(uuid)->val[6], sizeof(tmp2));
		memcpy(&tmp3, &BT_UUID_128(uuid)->val[8], sizeof(tmp3));
		memcpy(&tmp4, &BT_UUID_128(uuid)->val[10], sizeof(tmp4));
		memcpy(&tmp5, &BT_UUID_128(uuid)->val[12], sizeof(tmp5));

		snprintk(str, len, "%08x-%04x-%04x-%04x-%08x%04x",
			 sys_le32_to_cpu(tmp5), sys_le16_to_cpu(tmp4),
			 sys_le16_to_cpu(tmp3), sys_le16_to_cpu(tmp2),
			 sys_le32_to_cpu(tmp1), sys_le16_to_cpu(tmp0));
		break;
	default:
		(void)memset(str, 0, len);
		return;
	}
}

int esp_bt_map_attr(struct bt_gatt_attr const *attr, int svc_index, int chrc_index)
{
	struct bt_gatt_attr const *already = esp_bt_map_find_attr(svc_index, chrc_index);
	if (already && already == attr) {
		return 0;
	} else if (already) {
		return -EEXIST;
	}

	struct gatt_attr_db_ent *ent = NULL;
	int err = sys_mem_blocks_alloc(&gatt_attr_db_pool, 1, (void**)&ent);
	if (err) {
		return err;
	}

	memset(ent, 0, sizeof(*ent));
	ent->attr = attr;
	ent->chrc_index = chrc_index;
	ent->svc_index = svc_index;

	rb_insert(&gatt_attr_db, &ent->node);
	return 0;
}

struct bt_gatt_attr const *esp_bt_map_find_attr(int svc_index, int chrc_index)
{
	struct gatt_attr_db_ent *ent;
	RB_FOR_EACH_CONTAINER(&gatt_attr_db, ent, node) {
		if (ent->chrc_index == chrc_index && ent->svc_index == svc_index) {
			return ent->attr;
		}
	}
	return NULL;
}

bool esp_bt_map_find_attr_index(struct bt_gatt_attr const *attr, int *svc_index, int *chrc_index)
{
	struct gatt_attr_db_ent *ent;
	RB_FOR_EACH_CONTAINER(&gatt_attr_db, ent, node) {
		if (ent->attr == attr) {
			if (svc_index) {
				*svc_index = ent->svc_index;
			}
			if (chrc_index) {
				*chrc_index = ent->chrc_index;
			}
			return true;
		}
	}
	return false;
}


void esp_on_bt_connected(struct k_work *work)
{
	struct bt_conn *conn = CONTAINER_OF(work, struct bt_conn, bt_connected_work);

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->connected) {
			cb->connected(conn, 0);
		}
	}
}

void esp_on_bt_disconnected(struct k_work *work)
{
	struct bt_conn *conn = CONTAINER_OF(work, struct bt_conn, bt_disconnected_work);

	STRUCT_SECTION_FOREACH(bt_conn_cb, cb) {
		if (cb->disconnected) {
			cb->disconnected(conn, 0);
		}
	}
}

static bool gatt_attr_db_cmp(struct rbnode *a, struct rbnode *b)
{
	struct gatt_attr_db_ent *ea = CONTAINER_OF(a, struct gatt_attr_db_ent, node);
	struct gatt_attr_db_ent *eb = CONTAINER_OF(b, struct gatt_attr_db_ent, node);

	int d = ea->svc_index - eb->svc_index;
	if (d < 0) {
		return true;
	} else if (d > 0) {
		return false;
	}

	d = ea->chrc_index - eb->chrc_index;
	return d < 0;
}

#endif
