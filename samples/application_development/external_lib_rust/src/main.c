#include <zephyr/kernel.h>
#include <mylib/bindings.h>

int main()
{
	Sum sum;
	sum.x = 123;
	sum.y = 234;
	print_sum(&sum);

	return 0;
}

void rust_panic()
{
	k_panic();
}
