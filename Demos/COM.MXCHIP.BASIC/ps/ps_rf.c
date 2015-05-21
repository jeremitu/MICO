#include "MICO.h"
#include "MICORTOS.h"

#define ps_rf_log(M, ...) custom_log("PS", M, ##__VA_ARGS__)
#define ps_rf_trace() custom_log_trace("PS")


void start_rf_mcu( void )
{
  ps_rf_log("start rf powersave mode");
  micoWlanEnablePowerSave( );
}

void stop_rf_mcu( void )
{
  ps_rf_log("stop rf powersave mode");
  micoWlanDisablePowerSave( );
}
