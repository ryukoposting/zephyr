#![no_std]

pub mod zephyr;

#[repr(C)]
pub struct Sum {
    x: i32,
    y: i32
}

#[no_mangle]
pub extern fn print_sum(sum: &Sum) {
    println!("The sum of {} and {} is {}!", sum.x, sum.y, sum.x + sum.y);
}
