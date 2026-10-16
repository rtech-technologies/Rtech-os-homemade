use alloc::string::String;
use alloc::vec::Vec;
use spin::Mutex;

#[derive(Debug, Clone)]
pub struct File {
    pub name: String,
    pub content: Vec<u8>,
}

pub struct RamDisk {
    pub files: Vec<File>,
}

impl RamDisk {
    pub const fn new() -> Self {
        RamDisk { files: Vec::new() }
    }

    pub fn create_file(&mut self, name: &str, content: &[u8]) {
        self.files.push(File {
            name: String::from(name),
            content: content.to_vec(),
        });
    }

    pub fn read_file(&self, name: &str) -> Option<&[u8]> {
        for file in &self.files {
            if file.name == name {
                return Some(&file.content);
            }
        }
        None
    }
}

use lazy_static::lazy_static;
lazy_static! {
    pub static ref RAMDISK: Mutex<RamDisk> = Mutex::new(RamDisk::new());
}
