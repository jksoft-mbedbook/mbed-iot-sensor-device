#include "mbed.h"
#include "http_request.h"
#include "BME280.h"

// センサ値のアップロード周期
#define SENSOR_UPDATE_RATE_MS   60 * 1000

// センサデバイス固有ID
#define ID                      "1"
// HerokuのApp name
#define HEROKU_APP_NAME         "sensor-data-hub"

BME280 sensor(I2C_SDA, I2C_SCL);

static NetworkInterface *network = NULL;

// floaの文字列に変換
char* float_to_char(char* c,float value, int number_of_digits)
{
    int dd = 1;

    for(int i=0;i<number_of_digits;i++){
        dd *= 10;
    }

    int d = (int)value;
    int f = (int)(value * dd) % dd;

    sprintf(c,"%d.%d",d,f);

     return c;
}

int main() {
    int status;
    
    printf("Connect to network\n");
    network = NetworkInterface::get_default_instance();
    if (network == NULL) {
        printf("Failed to get default NetworkInterface\n");
        return -1;
    }
    status = network->connect();
    if (status != NSAPI_ERROR_OK) {
        printf("NetworkInterface failed to connect with %d\n", status);
        return -1;
    }
    SocketAddress sa;
    status = network->get_ip_address(&sa);
    if (status!=0) {
        printf("get_ip_address failed with %d\n", status);
        return -2;
    }
    printf("Network initialized, connected with IP %s\n\n", sa.get_ip_address());

    while (true) {
        char temperature[10];
        char pressure[10];
        char humidity[10];
        char body[100];

        // センサ値を取得して文字列に変換
        float_to_char(temperature,sensor.getTemperature(),1);
        float_to_char(pressure,sensor.getPressure(),1);
        float_to_char(humidity,sensor.getHumidity(),1);

        // JSON型に変換
        sprintf(body,"{\"id\":\"%s\",\"temperature\":%s,\"pressure\":%s,\"humidity\":%s}",ID,temperature,pressure,humidity);

        HttpRequest* request = new HttpRequest(network, HTTP_POST, "http://"  HEROKU_APP_NAME  ".herokuapp.com/push");
        request->set_header("Content-Type", "application/json");

        // HerokuにPOSTする
        HttpResponse* http_res = request->send(body,strlen(body));
        if(!http_res) {
            printf("http post error(status:%d)\n",request->get_error());
        } else {
            printf("http post success(status:%d)\n",http_res->get_status_code());
        }
        delete request;

        thread_sleep_for(SENSOR_UPDATE_RATE_MS);
    }
}