#include "../../includes/win32def.h"
#include "Plugin.h"

Plugin::Plugin(void)
{
}

Plugin::~Plugin(void)
{
}

void Plugin::release(void)
{
	delete this;
}
