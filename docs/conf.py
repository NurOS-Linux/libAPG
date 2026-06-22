project = "libapg"
author = "AnmiTaliDev"
release = "1.9.0"
copyright = "2026, AnmiTaliDev"

extensions = [
    "breathe",
    "sphinx.ext.intersphinx",
]

breathe_projects = {"libapg": "../docs/_doxygen/xml"}
breathe_default_project = "libapg"
breathe_domain_by_extension = {"h": "c"}

breathe_default_members = ("members", "undoc-members")

html_theme = "furo"
html_title = "libapg 1.5.0"

intersphinx_mapping = {}
