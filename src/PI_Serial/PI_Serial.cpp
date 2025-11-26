// #define isDEBUG
#include "ArduinoJson.h"
#include "PI_Serial.h"

#include "CRC16.h"
#include "CRC.h"
CRC16 crc;
// static
#include "QPI.h"
#include "QPIRI.h"
#include "QMN.h"
#include "QFLAG.h"
// variable
#include "Q1.h"
#include "QPIGS.h"
#include "QPIGS2.h"
#include "QMOD.h"
#include "QEX.h"
#include "QPIWS.h"
extern void writeLog(const char *format, ...);
//----------------------------------------------------------------------
//  Public Functions
//----------------------------------------------------------------------

PI_Serial::PI_Serial(int rx, int tx)
{
    this->my_serialIntf = new SoftwareSerial(rx, tx, false); // init pins
}

bool PI_Serial::Init()
{
    // Null check the serial interface
    if (this->my_serialIntf == NULL)
    {
        writeLog("No serial specificed!");
        return false;
    }
    autoDetect();
    if (isModbus())
    {
        if (requestCallback)
        {
            modbus->callback(requestCallback);
        }
        return true;
    }
    this->my_serialIntf->setTimeout(500);
    this->my_serialIntf->enableRxGPIOPullUp(true);
    this->my_serialIntf->begin(serialIntfBaud, SWSERIAL_8N1);
    return true;
}

bool PI_Serial::loop()
{
    if (millis() - previousTime > delayTime)
    {
        if (protocol != NoD)
        {
            if (sendCustomCommand())
            {
                requestStaticData = true;
                requestCounter = 0;
                previousTime = millis();
                return true;
            }
            if (isModbus())
            {
                modbus->loop();
            }
            else
            {
                switch (requestStaticData)
                {
                case true:
                    switch (requestCounter)
                    {
                    case 0:
                        requestCounter = PIXX_QPIRI() ? (requestCounter + 1) : 0;
                        break;
                    case 1:
                        requestCounter = PIXX_QMN() ? (requestCounter + 1) : 0;
                        break;
                    case 2:
                        requestCounter = PIXX_QPI() ? (requestCounter + 1) : 0;
                        break;
                    case 3:
                        requestCounter = PIXX_QFLAG() ? (requestCounter + 1) : 0;
                        requestCounter = 0;
                        requestStaticData = false;
                        break;
                    }
                    break;

                case false:
                    switch (requestCounter)
                    {
                    case 0:
                        requestCounter = PIXX_QPIGS() ? (requestCounter + 1) : 0;
                        break;
                    case 1:
                        requestCounter = PIXX_QPIGS2() ? (requestCounter + 1) : 0;
                        break;
                    case 2:
                        requestCounter = PIXX_QMOD() ? (requestCounter + 1) : 0;
                        break;
                    case 3:
                        requestCounter = PIXX_Q1() ? (requestCounter + 1) : 0;
                        break;
                    case 4:
                        requestCounter = PIXX_QEX() ? (requestCounter + 1) : 0;
                        break;
                    case 5:
                        requestCounter = PIXX_QPIWS() ? (requestCounter + 1) : 0;
                        break;
                    case 6:
                        requestCounter = PIXX_QPIRI() ? (requestCounter + 1) : 0;
                        break;
                    case 7:
                        requestCallback();
                        requestCounter = 0;
                        break;
                    }
                    break;
                }
            }
            if (isModbus())
            {
                connection = modbus->connection;
            }
            else
            {
                connection = (connectionCounter < 10) ? true : false;
            }
            previousTime = millis();
        }
        else
        {
            autoDetect();
            previousTime = millis();
            requestCallback();
            connection = false;
        }
    }
    return true;
}

void PI_Serial::callback(std::function<void()> func)
{
    requestCallback = func;
}

String PI_Serial::sendCommand(String command)
{
    if (command == "")
    {
        return command;
    }
    customCommandBuffer = command;
    return command;
}
//----------------------------------------------------------------------
// Private Functions
//----------------------------------------------------------------------
void PI_Serial::autoDetect() // function for autodetect the inverter type
{
    String protocolName = "not found";
    writeLog("----------------- Start Autodetect -----------------");
    for (size_t i = 0; i < 3; i++) // try 3 times to detect the inverter
    {
        writeLog("Try Autodetect Protocol");

        startChar = "(";
        serialIntfBaud = 2400;
        this->my_serialIntf->begin(serialIntfBaud, SWSERIAL_8N1);
        String qpi = this->requestData("QPI");
        writeLog("QPI:\t\t%s (Length: %d)", qpi, qpi.length());
        if (qpi != "" && qpi.substring(0, 2) == "PI")
        {
            writeLog("<Autodetect> Match protocol: PI3X");
            delimiter = " ";
            protocol = PI30;
            break;
        }
        startChar = "^Dxxx";
        this->my_serialIntf->begin(serialIntfBaud, SWSERIAL_8N1);
        String P005PI = this->requestData("^P005PI");
        writeLog("^P005PI:\t\t%s (Length: %d)", P005PI, P005PI.length());
        if (P005PI != "" && P005PI == "18")
        {
            writeLog("<Autodetect> Match protocol: PI18");
            delimiter = ",";
            protocol = PI18;
            break;
        }
        this->my_serialIntf->end();
    }
    this->my_serialIntf->end();
    if (protocol == NoD)
    {
        modbus = new MODBUS(this->my_serialIntf);
        modbus->Init();
        if (modbus->autoDetect()){
            protocol = MODBUS_MUST;
        }
    } 
    writeLog("----------------- End Autodetect -----------------");
}

bool PI_Serial::sendCustomCommand()
{
    if (customCommandBuffer == "")
        return false;

    if (isModbus())
    {
        get.raw.commandAnswer = modbus->requestData(customCommandBuffer);
    }
    else
    {
        get.raw.commandAnswer = requestData(customCommandBuffer);
    }
    customCommandBuffer = "";
    return true;
}



String PI_Serial::requestData(String command)
{
    String commandBuffer = "";
    uint16_t crcCalc = 0;
    uint16_t crcRecive = 0;

    // 0. Flush RX buffer to remove old garbage
    while (this->my_serialIntf->available()) {
        this->my_serialIntf->read();
    }

    // 1. Send Command
    this->my_serialIntf->write(command.c_str());
    this->my_serialIntf->write(highByte(getCRC(command)));
    this->my_serialIntf->write(lowByte(getCRC(command)));
    this->my_serialIntf->write(0x0D);
    this->my_serialIntf->flush();

    // 2. Receive Data (Raw Read Loop)
    this->my_serialIntf->enableTx(true);
    delay(50); 

    String rawData = "";
    unsigned long startTime = millis();
    
    // Wait up to 1.5 seconds for long QPIRI responses
    while (millis() - startTime < 1500) 
    {
        if (this->my_serialIntf->available())
        {
            char c = this->my_serialIntf->read();
            
            // Collect EVERY byte, including Nulls (0x00)
            rawData += c;
            
            // Stop if we hit the terminator (Carriage Return)
            if (c == 0x0D) {
                break;
            }
            startTime = millis(); // Reset timeout
        }
        yield();
    }
    
    // Remove the trailing \r
    if (rawData.length() > 0 && rawData[rawData.length() - 1] == 0x0D) {
        rawData.remove(rawData.length() - 1);
    }
    
    this->my_serialIntf->enableTx(false);
    commandBuffer = rawData;

    // 3. Validation
    if (commandBuffer.length() > 2) 
    {
        // Extract Data and CRC
        String dataPayload = commandBuffer.substring(0, commandBuffer.length() - 2);
        
        uint8_t crcHigh = (uint8_t)commandBuffer[commandBuffer.length() - 2];
        uint8_t crcLow  = (uint8_t)commandBuffer[commandBuffer.length() - 1];
        uint16_t crcReceivedVal = (uint16_t)crcHigh * 256 + crcLow;
        
        // Check CRC on the RAW data (with Nulls)
        if (getCRC(dataPayload) == crcReceivedVal)
        {
            // CRC Passed! Now clean the data for the parser.
            
            // Remove CRC bytes
            commandBuffer.remove(commandBuffer.length() - 2); 
            
            // Remove Start Char
            if (commandBuffer.startsWith(startChar)) {
                 commandBuffer.remove(0, strlen(startChar));
            }

            // --- IMPORTANT: NOW remove Null Bytes so the parser works ---
            String cleanBuffer = "";
            for (unsigned int i = 0; i < commandBuffer.length(); i++) {
                if (commandBuffer[i] != 0x00) {
                    cleanBuffer += commandBuffer[i];
                }
            }
            commandBuffer = cleanBuffer;
            // ------------------------------------------------------------

            connectionCounter = 0;
        }
        else if (commandBuffer.indexOf("NAK") >= 0)
        {
            commandBuffer = "NAK";
        }
        else 
        {
            // Log mismatch to help debug
            writeLog("ERROR CRC: >%s< (Len: %d, Calc: %04X, Rec: %04X)", 
                     command.c_str(), 
                     commandBuffer.length(),
                     getCRC(dataPayload), 
                     crcReceivedVal);
            connectionCounter++;
            commandBuffer = "ERCRC";
        }
    }
    else
    {
        commandBuffer = "NAK";
    }

    writeLog("[C: %s][L: %u]", command.c_str(), commandBuffer.length());
    return commandBuffer;
}

uint16_t cal_crc_half(uint8_t *pin, uint8_t len)
// https://forums.aeva.asn.au/viewtopic.php?t=4332&start=25
{
    uint16_t crc;
    uint8_t da;
    uint8_t *ptr;
    uint8_t bCRCHign;
    uint8_t bCRCLow;
    uint16_t crc_ta[16] =
        {
            0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
            0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef};
    ptr = pin;
    crc = 0;
    while (len-- != 0)
    {
        da = static_cast<uint8_t>((crc >> 8)) >> 4;
        crc <<= 4;
        crc ^= crc_ta[da ^ (*ptr >> 4)];
        da = static_cast<uint8_t>((crc >> 8)) >> 4;
        crc <<= 4;
        crc ^= crc_ta[da ^ (*ptr & 0x0f)];
        ptr++;
    }
    bCRCLow = crc;
    bCRCHign = static_cast<uint8_t>(crc >> 8);
    if (bCRCLow == 0x28 || bCRCLow == 0x0d || bCRCLow == 0x0a)
    {
        bCRCLow++;
    }
    if (bCRCHign == 0x28 || bCRCHign == 0x0d || bCRCHign == 0x0a)
    {
        bCRCHign++;
    }
    crc = static_cast<uint16_t>(bCRCHign) << 8;
    crc += bCRCLow;
    return crc;
}

uint16_t PI_Serial::getCRC(String data) // get a calculated crc from a string
{
    crc.reset();
    crc.setPolynome(0x1021);
    crc.add((uint8_t *)data.c_str(), data.length());
    // return crc.calc(); // here comes the crc;
    crc.calc();
    uint8_t CRCLow = lowByte(crc.calc());
    uint8_t CRCHigh = highByte(crc.calc());
    uint16_t CRCre;
    if (CRCLow == 0x28 || CRCLow == 0x0d || CRCLow == 0x0a)
    {
        CRCLow++;
    }
    if (CRCHigh == 0x28 || CRCHigh == 0x0d || CRCHigh == 0x0a)
    {
        CRCHigh++;
    }
    CRCre = ((uint16_t)CRCHigh) << 8;
    CRCre += CRCLow;
    return (CRCre);
}

byte PI_Serial::getCHK(String data) // get a calculatedt CHK
{
    byte chk = 0;
    for (unsigned int i = 0; i < data.length(); i++)
    {
        chk += data[i];
    }
    return chk;
}

char *PI_Serial::getModeDesc(char mode) // get the char from QMOD and make readable things
{
    char *modeString;
    switch (mode)
    {
    default:
        modeString = (char *)("");
        break;
    case 'P':
        modeString = (char *)"Power On";
        break;
    case 'S':
        modeString = (char *)"Standby";
        break;
    case 'Y':
        modeString = (char *)"Bypass";
        break;
    case 'L':
        modeString = (char *)"Line";
        break;
    case 'B':
        modeString = (char *)"Battery";
        break;
    case 'T':
        modeString = (char *)"Battery Test";
        break;
    case 'F':
        modeString = (char *)"Fault";
        break;
    case 'D':
        modeString = (char *)"Shutdown";
        break;
    case 'G':
        modeString = (char *)"Grid";
        break;
    case 'C':
        modeString = (char *)"Charge";
        break;
    }
    return modeString;
}

bool PI_Serial::isModbus()
{
    return protocol == MODBUS_MUST;
}

bool PI_Serial::checkQFLAG(const String& flags, char symbol) {
    bool enabled = false;
    for (unsigned int i = 0; i < flags.length(); i++) {
        char c = flags.charAt(i);
        if (c == 'E') enabled = true;
        else if (c == 'D') enabled = false;
        else if (c == symbol) return enabled;
    }
    return false;
}
