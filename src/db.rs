use crate::Package;
use sled::{Db, IVec};
use std::path::Path;
use std::str;

pub struct PackageDb {
    db: Db
}

impl PackageDb {
    pub fn open<P: AsRef<Path>>(path: P) -> Result<Self, sled::Error> {
        let db = sled::open(path)?;
        Ok(PackageDb { db })
    }

    pub fn insert(&self, package: &Package) -> Result<(), Box<dyn std::error::Error>> {
        let key = package.full_id();
        let value = package.to_complete_json()?;
        self.db.insert(key, value.as_bytes())?;
        Ok(())
    }

    pub fn get(&self, full_id: &str) -> Result<Option<Package>, Box<dyn std::error::Error>> {
        if let Some(ivec) = self.db.get(full_id)? {
            let json_str = str::from_utf8(&*ivec.to_vec())?;

        }
    }
}
