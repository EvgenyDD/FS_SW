#ifndef SERIAL_H
#define SERIAL_H

#include <cstring>
#include <exception>
#include <limits>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) && !defined(__MINGW32__)

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;  // NOLINT
typedef unsigned short uint16_t;  // NOLINT
typedef int int32_t;
typedef unsigned int uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
// intptr_t and friends are defined in crtdefs.h through stdio.h.

#else

#include <stdint.h>

#endif

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <tchar.h>
#include <winsock2.h> // before Windows.h, else Winsock 1 conflict
#include <windows.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>
#include <cstring>

//using serial::PortInfo;
using std::string;
using std::vector;

static const DWORD port_name_max_length = 256;
static const DWORD friendly_name_max_length = 256;
static const DWORD hardware_id_max_length = 256;
#endif

#if defined(__linux__)
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include <glob.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using std::istringstream;
using std::ifstream;
using std::getline;
using std::vector;
using std::string;
using std::cout;
using std::endl;
#endif

#define THROW(exceptionClass, message) throw exceptionClass(__FILE__, __LINE__, (message))

namespace serial
{

/*!
 * Enumeration defines the possible bytesizes for the serial port.
 */
typedef enum
{
    fivebits =  5,
    sixbits =   6,
    sevenbits = 7,
    eightbits = 8
} bytesize_t;

/*!
 * Enumeration defines the possible parity types for the serial port.
 */
typedef enum
{
    parity_none =  0,
    parity_odd =   1,
    parity_even =  2,
    parity_mark =  3,
    parity_space = 4
} parity_t;

/*!
 * Enumeration defines the possible stopbit types for the serial port.
 */
typedef enum
{
    stopbits_one = 1,
    stopbits_two = 2,
    stopbits_one_point_five
} stopbits_t;

/*!
 * Enumeration defines the possible flowcontrol types for the serial port.
 */
typedef enum
{
    flowcontrol_none = 0,
    flowcontrol_software,
    flowcontrol_hardware
} flowcontrol_t;

/*!
 * Structure for setting the timeout of the serial port, times are
 * in milliseconds.
 *
 * In order to disable the interbyte timeout, set it to Timeout::max().
 */
struct Timeout
{
#ifdef max
#undef max
#endif
    static uint32_t max()
    {
        return std::numeric_limits<uint32_t>::max();
    }
    /*!
   * Convenience function to generate Timeout structs using a
   * single absolute timeout.
   *
   * \param timeout A long that defines the time in milliseconds until a
   * timeout occurs after a call to read or write is made.
   *
   * \return Timeout struct that represents this simple timeout provided.
   */
    static Timeout simpleTimeout(uint32_t timeout)
    {
        return Timeout(max(), timeout, 0, timeout, 0);
    }

    /*! Number of milliseconds between bytes received to timeout on. */
    uint32_t inter_byte_timeout;
    /*! A constant number of milliseconds to wait after calling read. */
    uint32_t read_timeout_constant;
    /*! A multiplier against the number of requested bytes to wait after
   *  calling read.
   */
    uint32_t read_timeout_multiplier;
    /*! A constant number of milliseconds to wait after calling write. */
    uint32_t write_timeout_constant;
    /*! A multiplier against the number of requested bytes to wait after
   *  calling write.
   */
    uint32_t write_timeout_multiplier;

    explicit Timeout(uint32_t inter_byte_timeout_ = 0,
                     uint32_t read_timeout_constant_ = 0,
                     uint32_t read_timeout_multiplier_ = 0,
                     uint32_t write_timeout_constant_ = 0,
                     uint32_t write_timeout_multiplier_ = 0)
        : inter_byte_timeout(inter_byte_timeout_),
          read_timeout_constant(read_timeout_constant_),
          read_timeout_multiplier(read_timeout_multiplier_),
          write_timeout_constant(write_timeout_constant_),
          write_timeout_multiplier(write_timeout_multiplier_)
    {
    }
};

/*!
 * Class that provides a portable serial port interface.
 */
class Serial
{
public:
    /*!
   * Creates a Serial object and opens the port if a port is specified,
   * otherwise it remains closed until serial::Serial::open is called.
   *
   * \param port A std::string containing the address of the serial port,
   *        which would be something like 'COM1' on Windows and '/dev/ttyS0'
   *        on Linux.
   *
   * \param baudrate An unsigned 32-bit integer that represents the baudrate
   *
   * \param timeout A serial::Timeout struct that defines the timeout
   * conditions for the serial port. \see serial::Timeout
   *
   * \param bytesize Size of each byte in the serial transmission of data,
   * default is eightbits, possible values are: fivebits, sixbits, sevenbits,
   * eightbits
   *
   * \param parity Method of parity, default is parity_none, possible values
   * are: parity_none, parity_odd, parity_even
   *
   * \param stopbits Number of stop bits used, default is stopbits_one,
   * possible values are: stopbits_one, stopbits_one_point_five, stopbits_two
   *
   * \param flowcontrol Type of flowcontrol used, default is
   * flowcontrol_none, possible values are: flowcontrol_none,
   * flowcontrol_software, flowcontrol_hardware
   *
   * \throw serial::PortNotOpenedException
   * \throw serial::IOException
   * \throw std::invalid_argument
   */
    Serial(const std::string &port = "",
           uint32_t baudrate = 9600,
           Timeout timeout = Timeout(),
           bytesize_t bytesize = eightbits,
           parity_t parity = parity_none,
           stopbits_t stopbits = stopbits_one,
           flowcontrol_t flowcontrol = flowcontrol_none);

    virtual ~Serial();

    void open();
    bool isOpen() const;
    void close();
    size_t available();

    /*! Block until there is serial data to read or read_timeout_constant
   * number of milliseconds have elapsed. The return value is true when
   * the function exits with the port in a readable state, false otherwise
   * (due to timeout or select interruption). */
    bool waitReadable();

    /*! Block for a period of time corresponding to the transmission time of
   * count characters at present serial settings. This may be used in con-
   * junction with waitReadable to read larger blocks of data from the
   * port. */
    void waitByteTimes(size_t count);

    /*! Read a given amount of bytes from the serial port into a given buffer.
   *
   * The read function will return in one of three cases:
   *  * The number of requested bytes was read.
   *    * In this case the number of bytes requested will match the size_t
   *      returned by read.
   *  * A timeout occurred, in this case the number of bytes read will not
   *    match the amount requested, but no exception will be thrown.  One of
   *    two possible timeouts occurred:
   *    * The inter byte timeout expired, this means that number of
   *      milliseconds elapsed between receiving bytes from the serial port
   *      exceeded the inter byte timeout.
   *    * The total timeout expired, which is calculated by multiplying the
   *      read timeout multiplier by the number of requested bytes and then
   *      added to the read timeout constant.  If that total number of
   *      milliseconds elapses after the initial call to read a timeout will
   *      occur.
   *  * An exception occurred, in this case an actual exception will be thrown.
   *
   * \param buffer An uint8_t array of at least the requested size.
   * \param size A size_t defining how many bytes to be read.
   *
   * \return A size_t representing the number of bytes read as a result of the
   *         call to read.
   *
   * \throw serial::PortNotOpenedException
   * \throw serial::SerialException
   */
    size_t read(uint8_t *buffer, size_t size);

    /*! Read a given amount of bytes from the serial port into a give buffer.
   *
   * \param buffer A reference to a std::vector of uint8_t.
   * \param size A size_t defining how many bytes to be read.
   *
   * \return A size_t representing the number of bytes read as a result of the
   *         call to read.
   *
   * \throw serial::PortNotOpenedException
   * \throw serial::SerialException
   */
    size_t read(std::vector<uint8_t> &buffer);

    /*! Read a given amount of bytes from the serial port into a give buffer.
   *
   * \param buffer A reference to a std::string.
   * \param size A size_t defining how many bytes to be read.
   *
   * \return A size_t representing the number of bytes read as a result of the
   *         call to read.
   *
   * \throw serial::PortNotOpenedException
   * \throw serial::SerialException
   */
    size_t read(std::string &buffer);

    /*! Read a given amount of bytes from the serial port and return a string
   *  containing the data.
   *
   * \param size A size_t defining how many bytes to be read.
   *
   * \return A std::string containing the data read from the port.
   *
   * \throw serial::PortNotOpenedException
   * \throw serial::SerialException
   */
    std::string read(size_t size = 1);

    /*! Reads in a line or until a given delimiter has been processed.
   *
   * Reads from the serial port until a single line has been read.
   *
   * \param buffer A std::string reference used to store the data.
   * \param size A maximum length of a line, defaults to 65536 (2^16)
   * \param eol A string to match against for the EOL.
   *
   * \return A size_t representing the number of bytes read.
   *
   * \throw serial::PortNotOpenedException
   * \throw serial::SerialException
   */
    size_t readline(std::string &buffer, size_t size = 65536, std::string eol = "\n");

    /*! Reads in a line or until a given delimiter has been processed.
   *
   * Reads from the serial port until a single line has been read.
   *
   * \param size A maximum length of a line, defaults to 65536 (2^16)
   * \param eol A string to match against for the EOL.
   *
   * \return A std::string containing the line.
   *
   * \throw serial::PortNotOpenedException
   * \throw serial::SerialException
   */
    std::string readline(size_t size = 65536, std::string eol = "\n");

    /*! Reads in multiple lines until the serial port times out.
   *
   * This requires a timeout > 0 before it can be run. It will read until a
   * timeout occurs and return a list of strings.
   *
   * \param size A maximum length of combined lines, defaults to 65536 (2^16)
   *
   * \param eol A string to match against for the EOL.
   *
   * \return A vector<string> containing the lines.
   *
   * \throw serial::PortNotOpenedException
   * \throw serial::SerialException
   */
    std::vector<std::string> readlines(size_t size = 65536, std::string eol = "\n");

    /*! Write a string to the serial port.
   *
   * \param data A const reference containing the data to be written
   * to the serial port.
   *
   * \param size A size_t that indicates how many bytes should be written from
   * the given data buffer.
   *
   * \return A size_t representing the number of bytes actually written to
   * the serial port.
   *
   * \throw serial::PortNotOpenedException
   * \throw serial::SerialException
   * \throw serial::IOException
   */
    size_t write(const uint8_t *data, size_t size);

    /*! Write a string to the serial port.
   *
   * \param data A const reference containing the data to be written
   * to the serial port.
   *
   * \return A size_t representing the number of bytes actually written to
   * the serial port.
   *
   * \throw serial::PortNotOpenedException
   * \throw serial::SerialException
   * \throw serial::IOException
   */
    size_t write(const std::vector<uint8_t> &data);

    /*! Write a string to the serial port.
   *
   * \param data A const reference containing the data to be written
   * to the serial port.
   *
   * \return A size_t representing the number of bytes actually written to
   * the serial port.
   *
   * \throw serial::PortNotOpenedException
   * \throw serial::SerialException
   * \throw serial::IOException
   */
    size_t write(const std::string &data);

    /*! Sets the serial port identifier.
   *
   * \param port A const std::string reference containing the address of the
   * serial port, which would be something like 'COM1' on Windows and
   * '/dev/ttyS0' on Linux.
   *
   * \throw std::invalid_argument
   */
    void setPort(const std::string &port);

    /*! Gets the serial port identifier.
   *
   * \see Serial::setPort
   *
   * \throw std::invalid_argument
   */
    std::string getPort() const;

    /*! Sets the timeout for reads and writes using the Timeout struct.
   *
   * There are two timeout conditions described here:
   *  * The inter byte timeout:
   *    * The inter_byte_timeout component of serial::Timeout defines the
   *      maximum amount of time, in milliseconds, between receiving bytes on
   *      the serial port that can pass before a timeout occurs.  Setting this
   *      to zero will prevent inter byte timeouts from occurring.
   *  * Total time timeout:
   *    * The constant and multiplier component of this timeout condition,
   *      for both read and write, are defined in serial::Timeout.  This
   *      timeout occurs if the total time since the read or write call was
   *      made exceeds the specified time in milliseconds.
   *    * The limit is defined by multiplying the multiplier component by the
   *      number of requested bytes and adding that product to the constant
   *      component.  In this way if you want a read call, for example, to
   *      timeout after exactly one second regardless of the number of bytes
   *      you asked for then set the read_timeout_constant component of
   *      serial::Timeout to 1000 and the read_timeout_multiplier to zero.
   *      This timeout condition can be used in conjunction with the inter
   *      byte timeout condition with out any problems, timeout will simply
   *      occur when one of the two timeout conditions is met.  This allows
   *      users to have maximum control over the trade-off between
   *      responsiveness and efficiency.
   *
   * Read and write functions will return in one of three cases.  When the
   * reading or writing is complete, when a timeout occurs, or when an
   * exception occurs.
   *
   * A timeout of 0 enables non-blocking mode.
   *
   * \param timeout A serial::Timeout struct containing the inter byte
   * timeout, and the read and write timeout constants and multipliers.
   *
   * \see serial::Timeout
   */
    void setTimeout(Timeout &timeout);

    /*! Sets the timeout for reads and writes. */
    void setTimeout(uint32_t inter_byte_timeout, uint32_t read_timeout_constant,
                    uint32_t read_timeout_multiplier, uint32_t write_timeout_constant,
                    uint32_t write_timeout_multiplier)
    {
        Timeout timeout(inter_byte_timeout, read_timeout_constant,
                        read_timeout_multiplier, write_timeout_constant,
                        write_timeout_multiplier);
        return setTimeout(timeout);
    }

    /*! Gets the timeout for reads in seconds.
   *
   * \return A Timeout struct containing the inter_byte_timeout, and read
   * and write timeout constants and multipliers.
   *
   * \see Serial::setTimeout
   */
    Timeout getTimeout() const;


    /*! Flush the input and output buffers */
    void flush();

    /*! Flush only the input buffer */
    void flushInput();

    /*! Flush only the output buffer */
    void flushOutput();


private:
    // Disable copy constructors
    Serial(const Serial &);
    Serial &operator=(const Serial &);

    // Pimpl idiom, d_pointer
    class SerialImpl;
    SerialImpl *pimpl_;

    // Scoped Lock Classes
    class ScopedReadLock;
    class ScopedWriteLock;

    // Read common function
    size_t
    read_(uint8_t *buffer, size_t size);
    // Write common function
    size_t
    write_(const uint8_t *data, size_t length);
};

class SerialException : public std::exception
{
    // Disable copy constructors
    SerialException &operator=(const SerialException &);
    std::string e_what_;

public:
    SerialException(const char *description)
    {
        std::stringstream ss;
        ss << "SerialException " << description << " failed.";
        e_what_ = ss.str();
    }
    SerialException(const SerialException &other) : e_what_(other.e_what_) {}
    virtual ~SerialException() noexcept {}
    virtual const char *what() const noexcept
    {
        return e_what_.c_str();
    }
};

class IOException : public std::exception
{
    // Disable copy constructors
    IOException &operator=(const IOException &);
    std::string file_;
    int line_;
    std::string e_what_;
    int errno_;

public:
    explicit IOException(std::string file, int line, int errnum)
        : file_(file), line_(line), errno_(errnum)
    {
        std::stringstream ss;
#if defined(_WIN32) && !defined(__MINGW32__)
        char error_str[1024];
        strerror_s(error_str, 1024, errnum);
#else
        char *error_str = strerror(errnum);
#endif
        ss << "IO Exception (" << errno_ << "): " << error_str;
        ss << ", file " << file_ << ", line " << line_ << ".";
        e_what_ = ss.str();
    }
    explicit IOException(std::string file, int line, const char *description)
        : file_(file), line_(line), errno_(0)
    {
        std::stringstream ss;
        ss << "IO Exception: " << description;
        ss << ", file " << file_ << ", line " << line_ << ".";
        e_what_ = ss.str();
    }
    virtual ~IOException() noexcept {}
    IOException(const IOException &other) : line_(other.line_), e_what_(other.e_what_), errno_(other.errno_) {}

    int getErrorNumber() const { return errno_; }

    virtual const char *what() const noexcept
    {
        return e_what_.c_str();
    }
};

class PortNotOpenedException : public std::exception
{
    // Disable copy constructors
    const PortNotOpenedException &operator=(PortNotOpenedException);
    std::string e_what_;

public:
    PortNotOpenedException(const char *description)
    {
        std::stringstream ss;
        ss << "PortNotOpenedException " << description << " failed.";
        e_what_ = ss.str();
    }
    PortNotOpenedException(const PortNotOpenedException &other) : e_what_(other.e_what_) {}
    virtual ~PortNotOpenedException() noexcept {}
    virtual const char *what() const noexcept
    {
        return e_what_.c_str();
    }
};

/*!
 * Structure that describes a serial device.
 */
struct PortInfo
{

    /*! Address of the serial port (this can be passed to the constructor of Serial). */
    std::string port;

    /*! Human readable description of serial device if available. */
    std::string description;

    /*! Hardware ID (e.g. VID:PID of USB serial devices) or "n/a" if not available. */
    std::string hardware_id;
};

/* Lists the serial ports available on the system
 *
 * Returns a vector of available serial ports, each represented
 * by a serial::PortInfo data structure:
 *
 * \return vector of serial::PortInfo.
 */


class PortList
{
public:
#if defined(_WIN32)
    static std::vector<PortInfo> list_ports()
    {
        // Convert a wide Unicode string to an UTF8 string
        auto utf8_encode = [](const std::wstring &wstr) -> std::string
        {
            int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), nullptr, 0, nullptr, nullptr);
            std::string strTo(size_needed, 0);
            WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, nullptr, nullptr);
            return strTo;
        };

        vector<PortInfo> devices_found;

        HDEVINFO device_info_set = SetupDiGetClassDevs(
                    (const GUID *)&GUID_DEVCLASS_PORTS,
                    nullptr,
                    nullptr,
                    DIGCF_PRESENT);

        unsigned int device_info_set_index = 0;
        SP_DEVINFO_DATA device_info_data;

        device_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

        while(SetupDiEnumDeviceInfo(device_info_set, device_info_set_index, &device_info_data))
        {
            device_info_set_index++;

            // Get port name
            HKEY hkey = SetupDiOpenDevRegKey(
                        device_info_set,
                        &device_info_data,
                        DICS_FLAG_GLOBAL,
                        0,
                        DIREG_DEV,
                        KEY_READ);

            TCHAR port_name[port_name_max_length];
            DWORD port_name_length = port_name_max_length;

            LONG return_code = RegQueryValueEx(
                        hkey,
                        _T("PortName"),
                        nullptr,
                        nullptr,
                        (LPBYTE)port_name,
                        &port_name_length);

            RegCloseKey(hkey);

            if(return_code != EXIT_SUCCESS)
                continue;

            if(port_name_length > 0 && port_name_length <= port_name_max_length)
                port_name[port_name_length - 1] = '\0';
            else
                port_name[0] = '\0';

            // Ignore parallel ports
            if(_tcsstr(port_name, _T("LPT")) != nullptr)
                continue;

            // Get port friendly name
            TCHAR friendly_name[friendly_name_max_length];
            DWORD friendly_name_actual_length = 0;

            BOOL got_friendly_name = SetupDiGetDeviceRegistryProperty(
                        device_info_set,
                        &device_info_data,
                        SPDRP_FRIENDLYNAME,
                        nullptr,
                        (PBYTE)friendly_name,
                        friendly_name_max_length,
                        &friendly_name_actual_length);

            if(got_friendly_name == TRUE && friendly_name_actual_length > 0)
                friendly_name[friendly_name_actual_length - 1] = '\0';
            else
                friendly_name[0] = '\0';

            // Get hardware ID
            TCHAR hardware_id[hardware_id_max_length];
            DWORD hardware_id_actual_length = 0;

            BOOL got_hardware_id = SetupDiGetDeviceRegistryProperty(
                        device_info_set,
                        &device_info_data,
                        SPDRP_HARDWAREID,
                        nullptr,
                        (PBYTE)hardware_id,
                        hardware_id_max_length,
                        &hardware_id_actual_length);

            if(got_hardware_id == TRUE && hardware_id_actual_length > 0)
                hardware_id[hardware_id_actual_length - 1] = '\0';
            else
                hardware_id[0] = '\0';

#ifdef UNICODE
            std::string portName = utf8_encode(port_name);
            std::string friendlyName = utf8_encode(friendly_name);
            std::string hardwareId = utf8_encode(hardware_id);
#else
            std::string portName = port_name;
            std::string friendlyName = friendly_name;
            std::string hardwareId = hardware_id;
#endif

            PortInfo port_entry;
            port_entry.port = portName;
            port_entry.description = friendlyName;
            port_entry.hardware_id = hardwareId;

            devices_found.push_back(port_entry);
        }

        SetupDiDestroyDeviceInfoList(device_info_set);

        return devices_found;
    }
#endif // #if defined(_WIN32)

#if defined(__linux__)
    static std::vector<std::string> glob_(const vector<string>& patterns)
    {
        vector<string> paths_found;

        if(patterns.size() == 0)
            return paths_found;

        glob_t glob_results;

        int glob_retval = glob(patterns[0].c_str(), 0, nullptr, &glob_results);

        vector<string>::const_iterator iter = patterns.begin();

        while(++iter != patterns.end())
        {
            glob_retval = glob(iter->c_str(), GLOB_APPEND, nullptr, &glob_results);
        }

        for(int path_index = 0; path_index < (int)glob_results.gl_pathc; path_index++)
        {
            paths_found.push_back(glob_results.gl_pathv[path_index]);
        }

        (void)glob_retval; // ?

        globfree(&glob_results);

        return paths_found;
    }

    static std::string basename(const string& path)
    {
        size_t pos = path.rfind("/");

        if(pos == std::string::npos)
            return path;

        return string(path, pos+1, string::npos);
    }

    static std::string dirname(const string& path)
    {
        size_t pos = path.rfind("/");

        if(pos == std::string::npos)
            return path;
        else if(pos == 0)
            return "/";

        return string(path, 0, pos);
    }

    static bool path_exists(const string& path)
    {
        struct stat sb;

        if( stat(path.c_str(), &sb ) == 0 )
            return true;

        return false;
    }

    static std::string realpath_(const string& path)
    {
        char* real_path = realpath(path.c_str(), nullptr);

        string result;

        if(real_path != nullptr)
        {
            result = real_path;

            free(real_path);
        }

        return result;
    }

    static std::string usb_sysfs_friendly_name(const string& sys_usb_path)
    {
        unsigned int device_number = 0;

        istringstream( read_line(sys_usb_path + "/devnum") ) >> device_number;

        string manufacturer = read_line( sys_usb_path + "/manufacturer" );

        string product = read_line( sys_usb_path + "/product" );

        string serial = read_line( sys_usb_path + "/serial" );

        if( manufacturer.empty() && product.empty() && serial.empty() )
            return "";

        return format("%s %s %s", manufacturer.c_str(), product.c_str(), serial.c_str() );
    }

    static std::vector<std::string> get_sysfs_info_(const string& device_path)
    {
        string device_name = basename( device_path );

        string friendly_name;

        string hardware_id;

        string sys_device_path = format( "/sys/class/tty/%s/device", device_name.c_str() );

        if( device_name.compare(0,6,"ttyUSB") == 0 )
        {
            sys_device_path = dirname( dirname( realpath_( sys_device_path ) ) );

            if( path_exists( sys_device_path ) )
            {
                friendly_name = usb_sysfs_friendly_name( sys_device_path );

                hardware_id = usb_sysfs_hw_string( sys_device_path );
            }
        }
        else if( device_name.compare(0,6,"ttyACM") == 0 )
        {
            sys_device_path = dirname( realpath_( sys_device_path ) );

            if( path_exists( sys_device_path ) )
            {
                friendly_name = usb_sysfs_friendly_name( sys_device_path );

                hardware_id = usb_sysfs_hw_string( sys_device_path );
            }
        }
        else
        {
            // Try to read ID string of PCI device

            string sys_id_path = sys_device_path + "/id";

            if( path_exists( sys_id_path ) )
                hardware_id = read_line( sys_id_path );
        }

        if( friendly_name.empty() )
            friendly_name = device_name;

        if( hardware_id.empty() )
            hardware_id = "n/a";

        vector<string> result;
        result.push_back(friendly_name);
        result.push_back(hardware_id);

        return result;
    }

    static std::string read_line(const string& file)
    {
        ifstream ifs(file.c_str(), ifstream::in);

        string line;

        if(ifs)
        {
            getline(ifs, line);
        }

        return line;
    }

    static std::string format(const char* format, ...)
    {
        va_list ap;

        size_t buffer_size_bytes = 256;

        string result;

        char* buffer = (char*)malloc(buffer_size_bytes);

        if( buffer == nullptr )
            return result;

        bool done = false;

        unsigned int loop_count = 0;

        while(!done)
        {
            va_start(ap, format);

            int return_value = vsnprintf(buffer, buffer_size_bytes, format, ap);

            if( return_value < 0 )
            {
                done = true;
            }
            else if( return_value >= (int)buffer_size_bytes )
            {
                // Realloc and try again.

                buffer_size_bytes = return_value + 1;

                char* new_buffer_ptr = (char*)realloc(buffer, buffer_size_bytes);

                if( new_buffer_ptr == nullptr )
                {
                    done = true;
                }
                else
                {
                    buffer = new_buffer_ptr;
                }
            }
            else
            {
                result = buffer;
                done = true;
            }

            va_end(ap);

            if( ++loop_count > 5 )
                done = true;
        }

        free(buffer);

        return result;
    }

    static std::string usb_sysfs_hw_string(const string& sysfs_path)
    {
        string serial_number = read_line( sysfs_path + "/serial" );

        if( serial_number.length() > 0 )
        {
            serial_number = format( "SNR=%s", serial_number.c_str() );
        }

        string vid = read_line( sysfs_path + "/idVendor" );

        string pid = read_line( sysfs_path + "/idProduct" );

        return format("USB VID:PID=%s:%s %s", vid.c_str(), pid.c_str(), serial_number.c_str() );
    }

    static std::vector<PortInfo> list_ports()
    {
        vector<PortInfo> results;

        vector<string> search_globs;
        search_globs.push_back("/dev/ttyACM*");
        search_globs.push_back("/dev/ttyS*");
        search_globs.push_back("/dev/ttyUSB*");
        search_globs.push_back("/dev/tty.*");
        search_globs.push_back("/dev/cu.*");

        vector<string> devices_found = glob_( search_globs );

        vector<string>::iterator iter = devices_found.begin();

        while( iter != devices_found.end() )
        {
            string device = *iter++;

            vector<string> sysfs_info = get_sysfs_info_( device );

            string friendly_name = sysfs_info[0];

            string hardware_id = sysfs_info[1];

            PortInfo device_entry;
            device_entry.port = device;
            device_entry.description = friendly_name;
            device_entry.hardware_id = hardware_id;

            results.push_back( device_entry );

        }

        return results;
    }
#endif // defined(__linux__)
};

} // namespace serial

#endif // SERIAL_H
