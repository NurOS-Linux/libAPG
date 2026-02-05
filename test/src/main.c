#include <stdio.h>
#include <apg/package.h>

int
main()
{
  struct package *pkg = package_new();
  pkg->installed_by_hand = true;
  install_package(pkg, "root" );
  package_free(pkg);
  return 0;
}
