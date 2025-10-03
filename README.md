## Freezer Data Logger (FDL) - 6x Thermocouple Test Mk1

This is a simple test Arduino Program for logging 6x Thermocouples.

### Wiring:

| GPIO # | SPI Line | SEN-30003-T Connections | MAX31856 |
| ------ | -------- | ----------------------- | -------- |
| GPIO6  | SPI_CLK  | SDK                     | SCK      |
| GPIO7  | SPI_D    |                         | SDI      |
| GPIO2  | SPI_Q    | SDO                     | SDO      |
| GPIO23 | SPICS_0  |                         | CS_0     |
| GPIO22 | SPICS_1  |                         | CS_1     |
| GPIO21 | SPICS_5  | CS3                     |          |
| GPIO20 | SPICS_4  | CS2                     |          |
| GPIO19 | SPICS_3  | CS1                     |          |
| GPIO18 | SPICS_2  | CS0                     |          |

![Breadboard test setup](https://github.com/jwidess/FDL-6xTC-Test/blob/main/BreadboardTest.png?raw=true)