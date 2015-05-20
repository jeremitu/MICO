#include "MICO.h"
#include "MICORTOS.h"

#define ps_mcu_log(M, ...) custom_log("PS", M, ##__VA_ARGS__)
#define ps_mcu_trace() custom_log_trace("PS")


void start_ps_mcu( void )
{
  ps_mcu_log("start mcu powersave mode");
  MicoMcuPowerSaveConfig(true);
}

void stop_ps_mcu( void )
{
  ps_mcu_log("stop mcu powersave mode");
  MicoMcuPowerSaveConfig(false);
}


