/*
 * ============================================================
 * SMART HOME + THINGSBOARD
 * ============================================================
 *
 * Sensors:
 *   MQ Sensor       -> AIO 6
 *   AHT25           -> I2C Bus 0, Address 0x38
 *
 * Actuator:
 *   LED             -> GPIO 12
 *
 * ThingsBoard:
 *   MQTT telemetry
 *   RPC LED control
 *
 * MQ threshold:
 *   MQ > 70  -> LED ON
 *   MQ <= 70 -> LED OFF
 *
 * Telemetry:
 *   mq_value
 *   smoke_detected
 *   temperature
 *   humidity
 *   led_status
 *
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "MQTTAsync.h"
#include "mraa.h"
#include "mraa/i2c.h"


/* ============================================================
 * THINGSBOARD SETTINGS
 * ============================================================
 */

#define ADDRESS         "tcp://mqtt.thingsboard.cloud:1883"

/*
 * PUT YOUR THINGSBOARD DEVICE ACCESS TOKEN HERE
 */
#define TOKEN           "YOUR_DEVICE_ACCESS_TOKEN"

#define RPC_SUB_TOPIC   "v1/devices/me/rpc/request/+"
#define TELEMETRY_TOPIC "v1/devices/me/telemetry"

#define QOS             1


/* ============================================================
 * HARDWARE SETTINGS
 * ============================================================
 */

/* MQ sensor analog input */
#define MQ_PIN          6

/* LED GPIO */
#define LED_GPIO_PIN    12

/* AHT25 */
#define I2C_BUS         0
#define AHT25_ADDR      0x38

/* MQ threshold */
#define MQ_THRESHOLD    70


/* ============================================================
 * GLOBAL VARIABLES
 * ============================================================
 */

static MQTTAsync client = NULL;

static mraa_aio_context mq = NULL;

static mraa_gpio_context led = NULL;

static mraa_i2c_context i2c = NULL;

static pthread_mutex_t mqtt_send_mutex =
    PTHREAD_MUTEX_INITIALIZER;

static volatile int running = 1;


/* ============================================================
 * AHT25 SENSOR FUNCTION
 * ============================================================
 */

int read_aht25(double *temperature, double *humidity)
{
    uint8_t cmd[3] = {
        0xAC,
        0x33,
        0x00
    };

    uint8_t data[6];


    /*
     * Send measurement command
     */
    if (mraa_i2c_write(i2c, cmd, 3) != MRAA_SUCCESS)
    {
        printf("[AHT25] Failed to send command\n");
        return -1;
    }


    /*
     * Wait for measurement
     */
    usleep(80000);


    /*
     * Read 6 bytes
     */
    if (mraa_i2c_read(i2c, data, 6) != 6)
    {
        printf("[AHT25] Read error\n");
        return -1;
    }


    /*
     * Humidity
     */
    uint32_t raw_humidity =
        ((uint32_t)data[1] << 12) |
        ((uint32_t)data[2] << 4) |
        ((uint32_t)data[3] >> 4);


    *humidity =
        ((double)raw_humidity * 100.0) /
        1048576.0;


    /*
     * Temperature
     */
    uint32_t raw_temperature =
        (((uint32_t)data[3] & 0x0F) << 16) |
        ((uint32_t)data[4] << 8) |
        data[5];


    *temperature =
        ((double)raw_temperature * 200.0) /
        1048576.0 - 50.0;


    return 0;
}


/* ============================================================
 * RPC HANDLER
 * ============================================================
 */

void handle_rpc_payload(const char *payload)
{
    printf("[RPC] Payload: %s\n", payload);


    const char *params_pos =
        strstr(payload, "\"params\"");


    if (params_pos == NULL)
    {
        printf("[RPC] No params field found\n");
        return;
    }


    const char *start =
        strchr(params_pos, ':');


    if (start == NULL)
    {
        printf("[RPC] Invalid RPC format\n");
        return;
    }


    start++;


    while (*start == ' ' ||
           *start == '\t' ||
           *start == '"')
    {
        start++;
    }


    /*
     * --------------------------------------------------------
     * ThingsBoard sends:
     *
     * {"method":"setLed","params":true}
     *
     * LED ON
     * --------------------------------------------------------
     */

    if (strncmp(start, "true", 4) == 0)
    {
        printf("[RPC] LED ON command received\n");

        mraa_gpio_write(led, 1);
    }


    /*
     * --------------------------------------------------------
     * LED OFF
     * --------------------------------------------------------
     */

    else if (strncmp(start, "false", 5) == 0)
    {
        printf("[RPC] LED OFF command received\n");

        mraa_gpio_write(led, 0);
    }


    else
    {
        printf("[RPC] Unsupported params value\n");
    }
}


/* ============================================================
 * MQTT MESSAGE ARRIVED
 * ============================================================
 */

int messageArrived(
    void *context,
    char *topicName,
    int topicLen,
    MQTTAsync_message *message)
{
    (void)context;
    (void)topicLen;


    char *payload =
        (char *)message->payload;


    printf("\n");
    printf("====================================\n");
    printf("[MQTT] Message received\n");
    printf("Topic   : %s\n", topicName);
    printf("Payload : %.*s\n",
           message->payloadlen,
           payload);
    printf("====================================\n");


    /*
     * Handle RPC
     */
    handle_rpc_payload(payload);


    /*
     * --------------------------------------------------------
     * RPC request topic:
     *
     * v1/devices/me/rpc/request/123
     *
     * Response:
     *
     * v1/devices/me/rpc/response/123
     * --------------------------------------------------------
     */

    if (strncmp(
            topicName,
            "v1/devices/me/rpc/request/",
            26) == 0)
    {
        const char *requestId =
            topicName + 26;


        char responseTopic[128];


        snprintf(
            responseTopic,
            sizeof(responseTopic),
            "v1/devices/me/rpc/response/%s",
            requestId
        );


        const char *responsePayload =
            "{\"result\":\"LED action executed successfully\"}";


        MQTTAsync_message pubmsg =
            MQTTAsync_message_initializer;


        pubmsg.payload =
            (char *)responsePayload;

        pubmsg.payloadlen =
            (int)strlen(responsePayload);

        pubmsg.qos =
            QOS;

        pubmsg.retained =
            0;


        MQTTAsync_responseOptions opts =
            MQTTAsync_responseOptions_initializer;


        pthread_mutex_lock(
            &mqtt_send_mutex
        );


        int rc =
            MQTTAsync_sendMessage(
                client,
                responseTopic,
                &pubmsg,
                &opts
            );


        pthread_mutex_unlock(
            &mqtt_send_mutex
        );


        if (rc == MQTTASYNC_SUCCESS)
        {
            printf(
                "[RPC] Response sent successfully\n"
            );
        }
        else
        {
            printf(
                "[RPC] Response failed, rc=%d\n",
                rc
            );
        }
    }


    MQTTAsync_freeMessage(
        &message
    );


    MQTTAsync_free(
        topicName
    );


    return 1;
}


/* ============================================================
 * MQTT CONNECT SUCCESS
 * ============================================================
 */

void onConnect(
    void *context,
    MQTTAsync_successData *response)
{
    (void)context;
    (void)response;


    printf("\n");
    printf("====================================\n");
    printf(" Connected to ThingsBoard\n");
    printf("====================================\n");


    /*
     * Subscribe to RPC
     */

    int rc =
        MQTTAsync_subscribe(
            client,
            RPC_SUB_TOPIC,
            QOS,
            NULL
        );


    if (rc == MQTTASYNC_SUCCESS)
    {
        printf(
            "[MQTT] RPC subscription successful\n"
        );
    }
    else
    {
        printf(
            "[MQTT] RPC subscription failed, rc=%d\n",
            rc
        );
    }
}


/* ============================================================
 * MQTT CONNECT FAILURE
 * ============================================================
 */

void onConnectFailure(
    void *context,
    MQTTAsync_failureData *response)
{
    (void)context;


    printf(
        "[MQTT] Connection failed, rc=%d\n",
        response ? response->code : 0
    );


    running = 0;
}


/* ============================================================
 * TELEMETRY THREAD
 * ============================================================
 */

void *telemetry_thread(void *arg)
{
    (void)arg;


    while (running)
    {
        /*
         * ====================================================
         * READ MQ SENSOR
         * ====================================================
         */

        int mq_value =
            mraa_aio_read(mq);


        if (mq_value < 0)
        {
            printf(
                "[MQ] Sensor read failed\n"
            );

            sleep(2);
            continue;
        }


        printf("\n");
        printf("------------------------------------\n");
        printf(
            "[MQ] Value: %d\n",
            mq_value
        );


        /*
         * ====================================================
         * MQ THRESHOLD
         *
         * > 70  -> LED ON
         * <= 70 -> LED OFF
         * ====================================================
         */

        int smoke_detected = 0;

        int led_status = 0;


        if (mq_value > MQ_THRESHOLD)
        {
            /*
             * Smoke/gas detected
             */

            smoke_detected = 1;

            led_status = 1;


            printf(
                "[MQ] SMOKE/GAS DETECTED!\n"
            );

            printf(
                "[LED] ON\n"
            );


            /*
             * LED ON
             */
            mraa_gpio_write(
                led,
                1
            );
        }
        else
        {
            /*
             * Normal
             */

            smoke_detected = 0;

            led_status = 0;


            printf(
                "[MQ] Normal\n"
            );

            printf(
                "[LED] OFF\n"
            );


            /*
             * LED OFF
             */
            mraa_gpio_write(
                led,
                0
            );
        }


        /*
         * ====================================================
         * READ AHT25
         * ====================================================
         */

        double temperature = 0.0;

        double humidity = 0.0;


        int aht_status =
            read_aht25(
                &temperature,
                &humidity
            );


        if (aht_status == 0)
        {
            printf(
                "[AHT25] Temperature: %.2f C\n",
                temperature
            );


            printf(
                "[AHT25] Humidity   : %.2f %%\n",
                humidity
            );
        }
        else
        {
            printf(
                "[AHT25] Read failed\n"
            );
        }


        /*
         * ====================================================
         * CREATE JSON TELEMETRY
         * ====================================================
         */

        char payload[512];


        if (aht_status == 0)
        {
            snprintf(
                payload,
                sizeof(payload),

                "{"
                "\"mq_value\":%d,"
                "\"smoke_detected\":%d,"
                "\"temperature\":%.2f,"
                "\"humidity\":%.2f,"
                "\"led_status\":%d"
                "}",

                mq_value,
                smoke_detected,
                temperature,
                humidity,
                led_status
            );
        }
        else
        {
            /*
             * AHT25 failed.
             * Send MQ data anyway.
             */

            snprintf(
                payload,
                sizeof(payload),

                "{"
                "\"mq_value\":%d,"
                "\"smoke_detected\":%d,"
                "\"led_status\":%d"
                "}",

                mq_value,
                smoke_detected,
                led_status
            );
        }


        /*
         * Display payload
         */

        printf(
            "[TB] Payload: %s\n",
            payload
        );


        /*
         * ====================================================
         * PUBLISH TO THINGSBOARD
         * ====================================================
         */

        MQTTAsync_message pubmsg =
            MQTTAsync_message_initializer;


        pubmsg.payload =
            payload;


        pubmsg.payloadlen =
            (int)strlen(payload);


        pubmsg.qos =
            QOS;


        pubmsg.retained =
            0;


        MQTTAsync_responseOptions opts =
            MQTTAsync_responseOptions_initializer;


        pthread_mutex_lock(
            &mqtt_send_mutex
        );


        int rc =
            MQTTAsync_sendMessage(
                client,
                TELEMETRY_TOPIC,
                &pubmsg,
                &opts
            );


        pthread_mutex_unlock(
            &mqtt_send_mutex
        );


        if (rc == MQTTASYNC_SUCCESS)
        {
            printf(
                "[TB] Telemetry uploaded successfully\n"
            );
        }
        else
        {
            printf(
                "[TB] Telemetry upload failed, rc=%d\n",
                rc
            );
        }


        printf(
            "------------------------------------\n"
        );


        /*
         * Wait 2 seconds
         */

        sleep(2);
    }


    return NULL;
}


/* ============================================================
 * SIGINT
 * ============================================================
 */

void handle_sigint(int signum)
{
    (void)signum;


    printf(
        "\nProgram stopped by user\n"
    );


    running = 0;
}


/* ============================================================
 * MAIN
 * ============================================================
 */

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;


    /*
     * ========================================================
     * SIGNAL
     * ========================================================
     */

    signal(
        SIGINT,
        handle_sigint
    );


    /*
     * ========================================================
     * MRAA INIT
     * ========================================================
     */

    if (mraa_init() != MRAA_SUCCESS)
    {
        printf(
            "MRAA initialization failed\n"
        );

        return 1;
    }


    /*
     * ========================================================
     * MQ SENSOR INIT
     * ========================================================
     */

    mq =
        mraa_aio_init(
            MQ_PIN
        );


    if (mq == NULL)
    {
        printf(
            "MQ initialization failed\n"
        );

        mraa_deinit();

        return 1;
    }


    /*
     * ========================================================
     * LED INIT
     * ========================================================
     */

    led =
        mraa_gpio_init(
            LED_GPIO_PIN
        );


    if (led == NULL)
    {
        printf(
            "LED initialization failed on GPIO %d\n",
            LED_GPIO_PIN
        );


        mraa_aio_close(
            mq
        );

        mraa_deinit();

        return 1;
    }


    /*
     * Set LED as output
     */

    mraa_gpio_dir(
        led,
        MRAA_GPIO_OUT
    );


    /*
     * LED OFF at startup
     */

    mraa_gpio_write(
        led,
        0
    );


    /*
     * ========================================================
     * AHT25 INIT
     * ========================================================
     */

    i2c =
        mraa_i2c_init(
            I2C_BUS
        );


    if (i2c == NULL)
    {
        printf(
            "AHT25 I2C initialization failed\n"
        );


        mraa_aio_close(
            mq
        );

        mraa_gpio_close(
            led
        );

        mraa_deinit();

        return 1;
    }


    /*
     * Set AHT25 address
     */

    mraa_i2c_address(
        i2c,
        AHT25_ADDR
    );


    /*
     * ========================================================
     * STARTUP MESSAGE
     * ========================================================
     */

    printf("\n");
    printf("====================================\n");
    printf("       SMART HOME SYSTEM\n");
    printf("====================================\n");

    printf(
        "MQ Sensor       : AIO %d\n",
        MQ_PIN
    );

    printf(
        "MQ Threshold    : %d\n",
        MQ_THRESHOLD
    );

    printf(
        "LED GPIO        : %d\n",
        LED_GPIO_PIN
    );

    printf(
        "AHT25 I2C Bus   : %d\n",
        I2C_BUS
    );

    printf(
        "AHT25 Address   : 0x%02X\n",
        AHT25_ADDR
    );

    printf(
        "====================================\n"
    );


    /*
     * ========================================================
     * CREATE MQTT CLIENT
     * ========================================================
     */

    int rc;


    rc =
        MQTTAsync_create(
            &client,
            ADDRESS,
            "smart-home-client",
            MQTTCLIENT_PERSISTENCE_NONE,
            NULL
        );


    if (rc != MQTTASYNC_SUCCESS)
    {
        printf(
            "MQTTAsync_create failed, rc=%d\n",
            rc
        );


        mraa_i2c_stop(
            i2c
        );

        mraa_aio_close(
            mq
        );

        mraa_gpio_close(
            led
        );

        mraa_deinit();

        return 1;
    }


    /*
     * ========================================================
     * MQTT CALLBACKS
     * ========================================================
     */

    rc =
        MQTTAsync_setCallbacks(
            client,
            NULL,
            NULL,
            messageArrived,
            NULL
        );


    if (rc != MQTTASYNC_SUCCESS)
    {
        printf(
            "MQTT callback setup failed, rc=%d\n",
            rc
        );


        MQTTAsync_destroy(
            &client
        );


        mraa_i2c_stop(
            i2c
        );

        mraa_aio_close(
            mq
        );

        mraa_gpio_close(
            led
        );

        mraa_deinit();

        return 1;
    }


    /*
     * ========================================================
     * MQTT CONNECTION OPTIONS
     * ========================================================
     */

    MQTTAsync_connectOptions conn_opts =
        MQTTAsync_connectOptions_initializer;


    conn_opts.keepAliveInterval = 60;

    conn_opts.cleansession = 1;


    /*
     * ThingsBoard access token
     * is the MQTT username
     */

    conn_opts.username =
        TOKEN;

    conn_opts.password =
        NULL;


    conn_opts.onSuccess =
        onConnect;


    conn_opts.onFailure =
        onConnectFailure;


    conn_opts.context =
        client;


    /*
     * ========================================================
     * CONNECT TO THINGSBOARD
     * ========================================================
     */

    printf(
        "\nConnecting to ThingsBoard...\n"
    );


    rc =
        MQTTAsync_connect(
            client,
            &conn_opts
        );


    if (rc != MQTTASYNC_SUCCESS)
    {
        printf(
            "MQTT connection failed, rc=%d\n",
            rc
        );


        MQTTAsync_destroy(
            &client
        );


        mraa_i2c_stop(
            i2c
        );

        mraa_aio_close(
            mq
        );

        mraa_gpio_close(
            led
        );

        mraa_deinit();

        return 1;
    }


    /*
     * ========================================================
     * START TELEMETRY THREAD
     * ========================================================
     */

    pthread_t tid;


    if (pthread_create(
            &tid,
            NULL,
            telemetry_thread,
            NULL) != 0)
    {
        perror(
            "Failed to create telemetry thread"
        );


        running = 0;
    }


    /*
     * ========================================================
     * MAIN LOOP
     * ========================================================
     */

    while (running)
    {
        sleep(1);
    }


    /*
     * ========================================================
     * SHUTDOWN
     * ========================================================
     */

    printf(
        "\nDisconnecting from ThingsBoard...\n"
    );


    MQTTAsync_disconnectOptions disc_opts =
        MQTTAsync_disconnectOptions_initializer;


    disc_opts.timeout =
        10000;


    MQTTAsync_disconnect(
        client,
        &disc_opts
    );


    /*
     * Wait for telemetry thread
     */

    pthread_join(
        tid,
        NULL
    );


    /*
     * Destroy MQTT client
     */

    MQTTAsync_destroy(
        &client
    );


    /*
     * ========================================================
     * HARDWARE CLEANUP
     * ========================================================
     */

    mraa_i2c_stop(
        i2c
    );


    mraa_aio_close(
        mq
    );


    mraa_gpio_close(
        led
    );


    mraa_deinit();


    printf(
        "Program exited successfully.\n"
    );


    return 0;
}