use core::fmt::Write;
use crate::zephyr::api;

pub struct Printk();

impl Write for Printk {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        let mut buf = heapless::Vec::<u8, 64>::new();

        s.as_bytes().chunks(63).for_each(|chunk| {
            unsafe { buf.set_len(chunk.len()) };
            buf.copy_from_slice(chunk);
            buf.push(0).unwrap();

            unsafe { api::printk(buf.as_ptr() as *const cty::c_char) };
        });
        
        Ok(())
    }
}

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => {{
        <$crate::zephyr::printk::Printk as core::fmt::Write>::write_fmt(&mut $crate::zephyr::printk::Printk(), core::format_args!($($arg)*))
            .unwrap();
    }};
}

#[macro_export]
macro_rules! println {
    () => {
        <$crate::zephyr::printk::Printk as core::fmt::Write>::write_str(&mut $crate::zephyr::printk::Printk(), "\n")
            .unwrap();
    };
    ($($arg:tt)*) => {{
        <$crate::zephyr::printk::Printk as core::fmt::Write>::write_fmt(&mut $crate::zephyr::printk::Printk(), core::format_args!($($arg)*))
            .unwrap();
        <$crate::zephyr::printk::Printk as core::fmt::Write>::write_str(&mut $crate::zephyr::printk::Printk(), "\n")
            .unwrap();
    }};
}
