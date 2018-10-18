
#ifndef AQ_MQTT_H_
#define AQ_MQTT_H_


#define AIR_TEMP_TOPIC "Temperature/Air"
#define POOL_TEMP_TOPIC "Temperature/Pool"
#define SPA_TEMP_TOPIC "Temperature/Spa"
//#define POOL_SETPT_TOPIC "Pool_Heater/setpoint"
//#define SPA_SETPT_TOPIC "Spa_Heater/setpoint"
#define SWG_PERCENT_TOPIC "SWG/Percent"
#define SWG_PERCENT_F_TOPIC "SWG/Percent_f"
#define SWG_PPM_TOPIC "SWG/PPM"
#define SWG_TOPIC "SWG"
#define POOL_THERMO_TEMP_TOPIC BTN_POOL_HTR "/Temperature"
#define SPA_THERMO_TEMP_TOPIC BTN_SPA_HTR "/Temperature"

#define AIR_TEMPERATURE   "Air"
#define POOL_TEMPERATURE  "Pool_Water"
#define SPA_TEMPERATURE   "Spa_Water"

#define AIR_TEMPERATURE_TOPIC AIR_TEMPERATURE "/Temperature"
#define POOL_TEMPERATURE_TOPIC POOL_TEMPERATURE "/Temperature"
#define SPA_TEMPERATURE_TOPIC SPA_TEMPERATURE "/Temperature"

#define SWG_ON 2
#define SWG_OFF 0

#define FREEZE_PROTECT "Freeze_Protect"

// These need to be moved, but at present only aq_mqtt uses them, so there are here.
// Also need to get the C values from aqualink manual and add those just incase
// someone has the controller set to C.
#define HEATER_MAX 104
#define MEATER_MIN 36
#define FREEZE_PT_MAX 42
#define FREEZE_PT_MIN 36
#define SWG_PERCENT_MAX 101
#define SWG_PERCENT_MIN 0
#endif // AQ_MQTT_H_
