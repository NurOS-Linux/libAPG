use std::collections::HashMap;
use std::fmt;
use serde::{Deserialize, Serialize};

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub enum PackageStatus {
    NotInstalled,
    Installed,
    UpdateAvailable,
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
    
    #[serde(skip)]
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
    #[serde(skip)]
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

    pub fn from_json(json: &str) -> Result<Self, serde_json::Error> {
        let mut package: Package = serde_json::from_str(json)?;
        package.status = PackageStatus::NotInstalled;
        package.metadata = HashMap::new();
        Ok(package)
    }

    pub fn from_metadata_file(path: &str) -> Result<Self, Box<dyn std::error::Error>> {
        let content = std::fs::read_to_string(path)?;
        Ok(Self::from_json(&content)?)
    }

    pub fn to_json(&self) -> Result<String, serde_json::Error> {
        serde_json::to_string_pretty(self)
    }

    pub fn save_metadata(&self, path: &str) -> Result<(), Box<dyn std::error::Error>> {
        let json = self.to_json()?;
        std::fs::write(path, json)?;
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
        
        write!(
            f,
            "{} {}{} - {}",
            self.name, self.version, arch_str, self.description
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
    fn test_package_from_json() {
        let json = r#"
        {
            "name": "apgexample",
            "version": "0.0.0",
            "architecture": "x86_64",
            "description": "The example APG package for NurOS (Tulpar).",
            "maintainer": "NurOS Developers",
            "license": "MIT",
            "homepage": "https://nuros.org",
            "dependencies": ["testapg2"],
            "conflicts": ["testapg3"],
            "provides": ["libtaliildar-dev"],
            "replaces": ["example-apg"]
        }
        "#;

        let pkg = Package::from_json(json).unwrap();
        assert_eq!(pkg.name, "apgexample");
        assert_eq!(pkg.version, "0.0.0");
        assert_eq!(pkg.architecture, Some("x86_64".to_string()));
        assert_eq!(pkg.license, Some("MIT".to_string()));
        assert_eq!(pkg.dependencies, vec!["testapg2"]);
        assert_eq!(pkg.conflicts, vec!["testapg3"]);
        assert_eq!(pkg.provides, vec!["libtaliildar-dev"]);
        assert_eq!(pkg.replaces, vec!["example-apg"]);
    }

    #[test]
    fn test_package_to_json() {
        let mut pkg = Package::new(
            "test-package".to_string(),
            "1.0.0".to_string(),
            "Test package".to_string(),
            "test@example.com".to_string(),
        );
        
        pkg.set_architecture(Some("x86_64".to_string()));
        pkg.set_license(Some("GPL-3.0".to_string()));
        pkg.add_dependency("libc".to_string());

        let json = pkg.to_json().unwrap();
        assert!(json.contains("\"name\": \"test-package\""));
        assert!(json.contains("\"architecture\": \"x86_64\""));
        assert!(json.contains("\"license\": \"GPL-3.0\""));
    }

    #[test]
    fn test_proprietary_package() {
        let pkg = Package::new(
            "proprietary-pkg".to_string(),
            "1.0".to_string(),
            "Proprietary software".to_string(),
            "company@example.com".to_string(),
        );

        assert!(pkg.is_proprietary());
    }

    #[test]
    fn test_architecture_independent() {
        let pkg = Package::new(
            "arch-independent".to_string(),
            "1.0".to_string(),
            "Architecture independent package".to_string(),
            "dev@example.com".to_string(),
        );

        assert!(pkg.is_architecture_independent());
        assert!(pkg.is_compatible_with("x86_64"));
        assert!(pkg.is_compatible_with("arm64"));
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
