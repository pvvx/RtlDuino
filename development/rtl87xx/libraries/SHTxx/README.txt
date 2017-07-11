Sensirion: An Arduino Library for the Sensirion SHT1x and SHT7x family of
temperature and humidity sensors.

Created by Markus Schatzl, November 28, 2008
Revised (v1.1) by Carl Jackson, August 4, 2010
Rewritten (v2.0) by Carl Jackson, December 8, 2010

Revision History

1.0 - Original code provides a constructor, two public functions, plus
      several private functions.  The primary public function, "measure",
      commands the sensor to perform both a temperature and a humidity
      measurement and then calculates the dewpoint.  Total execution time
      is approximately 400 milliseconds.

1.1 - Added several new functions while touching as little as possible of
      the original code.  The primary new feature is the ability to perform
      non-blocking measurements, ie, to return control to the calling routine
      after sending a command to the sensor rather than to spin waiting for
      the measurement to complete.  Also added the ability to set the sensor
      measurement precision (14-bit/12-bit Temp/RH vs. 12-bit/8-bit Temp/RH)
      for precision vs. speed trade-off.  Updated equation coefficients for
      the V4 version of the sensors per Sensirion recommendations.

2.0 - Extensive changes for robustness, code size, and new features.  Added
      CRC checking, consistent handling of the data pin internal pullup, and
      improved error reporting.  Added sensor status register read function
      and expanded status register write function to cover all setable bits.


Usage Information

CRC error detection is enabled by default.  It may be disabled by deleting
the line "#define CRC_ENA" in the library header file (Sensrion.h).  This
reduces the code size by about 150 bytes.  When enabled, CRC errors are
indicated via the return code S_Err_CRC.  In addition, the value 0xFFFF is
substituted for the affected data.

The library header file defines two macros (PULSE_SHORT and PULSE_LONG)
that are used in generating the sensor interface signaling.  By default,
PULSE_SHORT delays 1 microsecond and PULSE_LONG delays 3 microseconds.
These delays appear reliable for wire lengths of at least 1 foot (30 cm).
Long connections may require adjustments to the delay macros and may also
require serial termination, low pass filtering, or other approaches to
improve the sensor interface signal integrity.

To avoid self heating of the sensor, Sensirion recommends that the
sensor not be active for more than 10% of the time.  In its default high
resolution mode, the sensor performs 14-bit temperature measurements and
12-bit humidity measurements which may take up to 320 msec and 80 msec
respectively to complete.  One set of temperature / humidity measurements
thus takes up to 400 msec so at least 4 seconds should elapse between
successive sets of measurements.  In low resolution mode, the sensor
performs 12-bit temperature measurements and 8-bit humidity measurements,
reducing these times to 80 msec and 20 msec respectively for a total of
100 msec for a set.  In this case, at least 1 second should elapse between
successive sets of measurements.
