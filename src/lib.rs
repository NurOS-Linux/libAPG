mod db;

use std::collections::HashMap;
use std::fmt;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum PackageStatus {
    NotInstalled,
    UpdateAvailable,
    Installed,
    Broken,
}

impl Default for PackageStatus {
    fn default() -> Self {
        PackageStatus::NotInstalled
    }
}

#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Package {
    pub name: String,
    pub version: String,
    pub architecture: Option<String>,
    pub description: String,
    pub maintainer: String,
    pub license: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub homepage: Option<String>,
    #[serde(default)]
    pub dependencies: Vec<String>,
    #[serde(default)]
    pub conflicts: Vec<String>,
    #[serde(default)]
    pub provides: Vec<String>,
    #[serde(default)]
    pub replaces: Vec<String>,
    
    #[serde(default)]
    pub status: PackageStatus,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub installed_size: Option<u64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub download_size: Option<u64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub checksum: Option<String>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub download_url: Option<String>,
    #[serde(skip)]
    pub file_path: Option<String>,
    #[serde(default)]
    pub metadata: HashMap<String, String>,
}

impl Package {
    pub fn new(
        name: String,
        version: String,
        description: String,
        maintainer: String,
    ) -> Self {
        Self {
            name,
            version,
            architecture: None,
            description,
            maintainer,
            license: None,
            homepage: None,
            dependencies: Vec::new(),
            conflicts: Vec::new(),
            provides: Vec::new(),
            replaces: Vec::new(),
            status: PackageStatus::NotInstalled,
            installed_size: None,
            download_size: None,
            checksum: None,
            download_url: None,
            file_path: None,
            metadata: HashMap::new(),
        }
    }

    pub fn from_repository_json(json: &str) -> Result<Self, serde_json::Error> {
        let mut package: Package = serde_json::from_str(json)?;
        package.status = PackageStatus::NotInstalled;
        package.metadata.clear();
        package.file_path = None;
        Ok(package)
    }

    pub fn from_complete_json(json: &str) -> Result<Self, serde_json::Error> {
        serde_json::from_str(json)
    }

    pub fn from_json_with_options(
        json: &str, 
        preserve_status: bool, 
        preserve_metadata: bool
    ) -> Result<Self, serde_json::Error> {
        let mut package: Package = serde_json::from_str(json)?;
        
        if !preserve_status {
            package.status = PackageStatus::NotInstalled;
        }
        
        if !preserve_metadata {
            package.metadata.clear();
            package.file_path = None;
        }
        
        Ok(package)
    }

    pub fn from_repository_file(path: &str) -> Result<Self, Box<dyn std::error::Error>> {
        let content = std::fs::read_to_string(path)?;
        Ok(Self::from_repository_json(&content)?)
    }

    pub fn from_complete_file(path: &str) -> Result<Self, Box<dyn std::error::Error>> {
        let content = std::fs::read_to_string(path)?;
        Ok(Self::from_complete_json(&content)?)
    }

    pub fn to_repository_json(&self) -> Result<String, serde_json::Error> {
        let repo_package = Package {
            name: self.name.clone(),
            version: self.version.clone(),
            architecture: self.architecture.clone(),
            description: self.description.clone(),
            maintainer: self.maintainer.clone(),
            license: self.license.clone(),
            homepage: self.homepage.clone(),
            dependencies: self.dependencies.clone(),
            conflicts: self.conflicts.clone(),
            provides: self.provides.clone(),
            replaces: self.replaces.clone(),
            status: PackageStatus::NotInstalled,
            installed_size: self.installed_size,
            download_size: self.download_size,
            checksum: self.checksum.clone(),
            download_url: self.download_url.clone(),
            file_path: None,
            metadata: HashMap::new(),
        };
        serde_json::to_string_pretty(&repo_package)
    }

    pub fn to_complete_json(&self) -> Result<String, serde_json::Error> {
        serde_json::to_string_pretty(self)
    }

    pub fn save_repository_metadata(&self, path: &str) -> Result<(), Box<dyn std::error::Error>> {
        let json = self.to_repository_json()?;
        std::fs::write(path, json)?;
        Ok(())
    }

    pub fn save_complete_state(&self, path: &str) -> Result<(), Box<dyn std::error::Error>> {
        let json = self.to_complete_json()?;
        std::fs::write(path, json)?;
        Ok(())
    }

    pub fn as_repository_package(&self) -> Self {
        let mut repo_package = self.clone();
        repo_package.status = PackageStatus::NotInstalled;
        repo_package.metadata.clear();
        repo_package.file_path = None;
        repo_package
    }

    pub fn reset_local_state(&mut self) {
        self.status = PackageStatus::NotInstalled;
        self.metadata.clear();
        self.file_path = None;
    }

    pub fn merge_local_state(&mut self, other: &Package) -> Result<(), String> {
        if self.name != other.name || self.version != other.version {
            return Err("Cannot merge state from different package".to_string());
        }
        
        self.status = other.status.clone();
        self.metadata = other.metadata.clone();
        self.file_path = other.file_path.clone();
        Ok(())
    }

    pub fn is_installed(&self) -> bool {
        matches!(self.status, PackageStatus::Installed | PackageStatus::UpdateAvailable)
    }

    pub fn has_update(&self) -> bool {
        matches!(self.status, PackageStatus::UpdateAvailable)
    }

    pub fn is_broken(&self) -> bool {
        matches!(self.status, PackageStatus::Broken)
    }

    pub fn is_architecture_independent(&self) -> bool {
        self.architecture.is_none()
    }

    pub fn is_proprietary(&self) -> bool {
        self.license.is_none()
    }

    pub fn set_status(&mut self, status: PackageStatus) {
        self.status = status;
    }

    pub fn set_architecture(&mut self, arch: Option<String>) {
        self.architecture = arch;
    }

    pub fn set_license(&mut self, license: Option<String>) {
        self.license = license;
    }

    pub fn add_metadata(&mut self, key: String, value: String) {
        self.metadata.insert(key, value);
    }

    pub fn get_metadata(&self, key: &str) -> Option<&String> {
        self.metadata.get(key)
    }

    pub fn full_id(&self) -> String {
        match &self.architecture {
            Some(arch) => format!("{}_{}-{}", self.name, self.version, arch),
            None => format!("{}_{}", self.name, self.version),
        }
    }

    pub fn is_compatible_with(&self, target_arch: &str) -> bool {
        match &self.architecture {
            None => true,
            Some(arch) => arch == target_arch,
        }
    }

    pub fn add_dependency(&mut self, package_name: String) {
        if !self.dependencies.contains(&package_name) {
            self.dependencies.push(package_name);
        }
    }

    pub fn remove_dependency(&mut self, package_name: &str) {
        self.dependencies.retain(|dep| dep != package_name);
    }

    pub fn add_conflict(&mut self, package_name: String) {
        if !self.conflicts.contains(&package_name) {
            self.conflicts.push(package_name);
        }
    }

    pub fn remove_conflict(&mut self, package_name: &str) {
        self.conflicts.retain(|conflict| conflict != package_name);
    }

    pub fn add_provides(&mut self, capability: String) {
        if !self.provides.contains(&capability) {
            self.provides.push(capability);
        }
    }

    pub fn add_replaces(&mut self, package_name: String) {
        if !self.replaces.contains(&package_name) {
            self.replaces.push(package_name);
        }
    }

    pub fn depends_on(&self, package_name: &str) -> bool {
        self.dependencies.contains(&package_name.to_string())
    }

    pub fn conflicts_with(&self, package_name: &str) -> bool {
        self.conflicts.contains(&package_name.to_string())
    }

    pub fn provides_capability(&self, capability: &str) -> bool {
        self.provides.contains(&capability.to_string())
    }

    pub fn replaces_package(&self, package_name: &str) -> bool {
        self.replaces.contains(&package_name.to_string())
    }
}

impl fmt::Display for Package {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let arch_str = match &self.architecture {
            Some(arch) => format!(" ({})", arch),
            None => " (all)".to_string(),
        };
        
        let status_str = match self.status {
            PackageStatus::Installed => " [installed]",
            PackageStatus::UpdateAvailable => " [update available]",
            PackageStatus::Broken => " [broken]",
            PackageStatus::NotInstalled => "",
        };
        
        write!(
            f,
            "{} {}{} - {}{}",
            self.name, self.version, arch_str, self.description, status_str
        )
    }
}

impl PartialEq for Package {
    fn eq(&self, other: &Self) -> bool {
        self.name == other.name 
            && self.version == other.version 
            && self.architecture == other.architecture
    }
}

impl Eq for Package {}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_explicit_deserialization_methods() {
        let json_with_state = r#"
        {
            "name": "test-package",
            "version": "1.0.0",
            "description": "Test package",
            "maintainer": "test@example.com",
            "status": "Installed",
            "metadata": {
                "install_reason": "manual"
            }
        }
        "#;

        let repo_package = Package::from_repository_json(json_with_state).unwrap();
        assert_eq!(repo_package.status, PackageStatus::NotInstalled);
        assert!(repo_package.metadata.is_empty());

        let complete_package = Package::from_complete_json(json_with_state).unwrap();
        assert_eq!(complete_package.status, PackageStatus::Installed);
        assert_eq!(complete_package.get_metadata("install_reason"), Some(&"manual".to_string()));
    }

    #[test]
    fn test_serialization_variants() {
        let mut pkg = Package::new(
            "test".to_string(),
            "1.0".to_string(),
            "Test".to_string(),
            "test@test.com".to_string(),
        );
        
        pkg.set_status(PackageStatus::Installed);
        pkg.add_metadata("reason".to_string(), "manual".to_string());

        let repo_json = pkg.to_repository_json().unwrap();
        assert!(!repo_json.contains("Installed"));
        assert!(!repo_json.contains("reason"));

        let complete_json = pkg.to_complete_json().unwrap();
        assert!(complete_json.contains("Installed"));
        assert!(complete_json.contains("reason"));
    }

    #[test]
    fn test_state_management() {
        let mut pkg1 = Package::new(
            "test".to_string(),
            "1.0".to_string(),
            "Test".to_string(),
            "test@test.com".to_string(),
        );
        
        let mut pkg2 = pkg1.clone();
        pkg2.set_status(PackageStatus::Installed);
        pkg2.add_metadata("custom".to_string(), "value".to_string());

        pkg1.merge_local_state(&pkg2).unwrap();
        assert_eq!(pkg1.status, PackageStatus::Installed);
        assert_eq!(pkg1.get_metadata("custom"), Some(&"value".to_string()));

        pkg1.reset_local_state();
        assert_eq!(pkg1.status, PackageStatus::NotInstalled);
        assert!(pkg1.metadata.is_empty());
    }

    #[test]
    fn test_repository_package_creation() {
        let mut pkg = Package::new(
            "test".to_string(),
            "1.0".to_string(),
            "Test".to_string(),
            "test@test.com".to_string(),
        );
        
        pkg.set_status(PackageStatus::Installed);
        pkg.add_metadata("local".to_string(), "data".to_string());

        let repo_pkg = pkg.as_repository_package();
        assert_eq!(repo_pkg.name, pkg.name);
        assert_eq!(repo_pkg.status, PackageStatus::NotInstalled);
        assert!(repo_pkg.metadata.is_empty());
    }

    #[test]
    fn test_flexible_deserialization() {
        let json = r#"
        {
            "name": "test",
            "version": "1.0",
            "description": "Test",
            "maintainer": "test@test.com",
            "status": "Installed",
            "metadata": {"key": "value"}
        }
        "#;

        let pkg1 = Package::from_json_with_options(json, true, true).unwrap();
        assert_eq!(pkg1.status, PackageStatus::Installed);
        assert!(!pkg1.metadata.is_empty());

        let pkg2 = Package::from_json_with_options(json, true, false).unwrap();
        assert_eq!(pkg2.status, PackageStatus::Installed);
        assert!(pkg2.metadata.is_empty());

        let pkg3 = Package::from_json_with_options(json, false, false).unwrap();
        assert_eq!(pkg3.status, PackageStatus::NotInstalled);
        assert!(pkg3.metadata.is_empty());
    }

    #[test]
    fn test_dependency_management() {
        let mut pkg = Package::new(
            "test".to_string(),
            "1.0".to_string(),
            "Test".to_string(),
            "test@test.com".to_string(),
        );

        pkg.add_dependency("dep1".to_string());
        pkg.add_dependency("dep2".to_string());
        pkg.add_dependency("dep1".to_string()); 

        assert_eq!(pkg.dependencies.len(), 2);
        assert!(pkg.depends_on("dep1"));
        assert!(!pkg.depends_on("dep3"));

        pkg.remove_dependency("dep1");
        assert!(!pkg.depends_on("dep1"));
        assert_eq!(pkg.dependencies.len(), 1);
    }
}