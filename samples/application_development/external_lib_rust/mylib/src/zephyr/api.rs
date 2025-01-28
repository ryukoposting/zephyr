//! External functions implemented by Zephyr.

extern "C" {
    pub(crate) fn k_cpu_idle();

    pub(crate) fn k_cpu_atomic_idle(key: cty::c_uint);

    pub(crate) fn k_busy_wait(usec: cty::uint32_t);

    pub(crate) fn k_yield();

    pub(crate) fn printk(text: *const cty::c_char);
}
