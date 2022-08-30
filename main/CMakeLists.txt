idf_component_register(
    SRCS 
        "main.c" 
        "mqtt/mqtt.c"
        "networking/networking.c"
        "sensors/pressure/sensors_pressure.c"
        "sensors/flow/sensors_flow.c"
    INCLUDE_DIRS ""
)
