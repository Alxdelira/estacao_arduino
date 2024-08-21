#include <Ethernet.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <WeatherSensor.h>

#define RF_IN 2

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress server(138, 118, 76, 228);
EthernetClient client;

WeatherSensor weather;

class WeatherStation
{
public:
    void setup()
    {
        initializeTimer();
        Serial.begin(115200);
        Serial.println("Iniciado!");
        Serial.println("Aguardando dados da estação...");
        pinMode(RF_IN, INPUT);
        sei(); // Habilita interrupções

        int res = Ethernet.begin(mac);
        Serial.print("Resultado: -->");
        Serial.println(res);
    }

    void loop()
    {
        if (weather._interval)
        {
            processWeatherData();
            weather._interval = 0;
        }
        delay(1);
    }

private:
    void initializeTimer()
    {
        TCCR1A = 0x00;
        TCCR1B = 0x09;
        TCCR1C = 0x00;
        OCR1A = 399;
        TIMSK1 = 0x02;
    }

    void processWeatherData()
    {
        weather.Receiver(weather.interval);

        if (weather.acquired())
        {
            updateWeatherStats();
            printWeatherData();
            sendWeatherDataToServer();
        }
    }

    void updateWeatherStats()
    {
        weather.now = millis();
        weather.spacing = weather.now - weather.old;
        weather.old = weather.now;
        weather.packet_count++;
        weather.average_interval = weather.now / weather.packet_count;
    }

    void printWeatherData()
    {
        Serial.println("--------------------------------------------------------------");
        Serial.print("Tempo de intervalo de cada pacote recebido: ");
        Serial.println(weather.spacing, DEC);
        Serial.print("Pacotes recebidos: ");
        Serial.println(weather.packet_count, DEC);
        Serial.print("Sensor ID: 0x");
        Serial.println(weather.get_sensor_id(), HEX);
        Serial.print("Temperatura: ");
        Serial.print(weather.get_temperature());
        Serial.println(" C ");
        Serial.print("Humidade: ");
        Serial.print(weather.get_humidity());
        Serial.println(" %\t");
        Serial.print("Posição do vento: ");
        Serial.println(weather.get_wind_direction_str());
        Serial.print("Velocidade do vento: ");
        Serial.print(weather.get_wind_gust_kmh());
        Serial.println(" Kmh");
        Serial.print("Chuva: ");
        Serial.println(weather.get_rainfall());
        Serial.print("Bateria: ");
        Serial.println(weather.get_battery());
        Serial.println("--------------------------------------------------------------");
    }

    void sendWeatherDataToServer()
    {
        Serial.println("connecting...");

        if (client.connect(server, 6097))
        {
            Serial.println("connected");

            StaticJsonDocument<200> doc;
            doc["spacing"] = weather.spacing;
            doc["packet_count"] = weather.packet_count;
            doc["sensor_id"] = weather.get_sensor_id();
            doc["temperature"] = weather.get_temperature();
            doc["humidity"] = weather.get_humidity();
            doc["wind_speed_kmh"] = weather.get_wind_gust_kmh();
            doc["rainfall"] = weather.get_rainfall();
            doc["battery"] = weather.get_battery();

            client.println("POST /temp HTTP/1.1");
            client.print("Host: ");
            client.println("138.118.76.228:6097");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.print("Content-Length: ");
            client.println(measureJson(doc));
            client.println();

            serializeJson(doc, client);

            delay(100);
            client.stop();
        }
        else
        {
            Serial.println("connection failed");
        }
    }
};

WeatherStation station;

ISR(TIMER1_COMPA_vect)
{
    static byte count = 0;
    static byte was_hi = 0;

    if (digitalRead(RF_IN) == HIGH)
    {
        count++;
        was_hi = 1;
    }
    else
    {
        if (was_hi)
        {
            was_hi = 0;
            weather.interval = count;
            weather._interval = 1;
            count = 0;
        }
    }
}

void setup()
{
    station.setup();
}

void loop()
{
    station.loop();
}
