use core::panic::PanicInfo;

#[macro_use]
pub mod printk;

/// cbindgen:ignore
mod api;
mod external;


#[inline]
pub fn cpu_idle() {
    unsafe { api::k_cpu_idle() };
}

#[inline]
pub fn cpu_atomic_idle(key: usize) {
    unsafe { api::k_cpu_atomic_idle(key as cty::c_uint) };
}

#[inline]
pub fn busy_wait(usec: u32) {
    unsafe { api::k_busy_wait(usec) };
}

#[inline]
pub fn thread_yield() {
    unsafe { api::k_yield() };
}

#[panic_handler]
pub fn panic(info: &PanicInfo) -> ! {
    println!("RUST PANIC: {}", info.message());
    if let Some(location) = info.location() {
        println!("@ {}:{}", location.file(), location.line());
    } else {
        println!("@ UNKNOWN LOCATION");
    }
    unsafe { external::rust_panic() };
}
