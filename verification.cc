#include <boost/process.hpp>
#include "verification.h"
#include "verification_dlg.h"
#include <QMessageBox>
#include "encryption.h"
#include "net_utils.h"
#include <memory>
#include <string_view>
#include <windows.h>
#include "json.hpp"
#include "algorithm_utils.h"
#include <boost/locale.hpp>
#include <string_view>
#include <QString>
#include <QDateTime>
#include <exception>
#include <base/io/file/file.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <base/utils/charset.hpp>
#include "json.hpp"
#include "krys_application.hpp"


#include <QFileInfo>
#define SERIAL_PATH "serial"
#define UPDATE_SERVER_ADDR "116.62.10.199"
#define DES_KEY "12345567890123456789video"
#define SERVER_ADDR "120.26.216.219"

using json = nlohmann::json;
using namespace std::string_literals;

using std::make_unique;
using std::string_view;
using std::string;

static std::string get_serial ();
static bool dlg_verification (const std::string& serial);

static bool local_verification (string_view serial, json data) noexcept
try
{
    const string history_serial = data ["serial"];
    const string last_datetime = data ["last_datetime"];
    const string expire_date = data ["expire_date"];
    const string account = data ["account"];

    const auto last = QDateTime::fromString (last_datetime.data (), "yyyy-MM-dd hh:mm:ss");
    const auto expire = QDateTime::fromString (expire_date.data (), "yyyy-MM-dd hh:mm:ss");
    auto current = QDateTime::currentDateTime ();
    if (last > current)
    {
        return false;
    }

    if (current > expire)
    {
        QMessageBox::information (NULL, "认证", "帐号已经过期");
        return false;
    }

    if (serial != history_serial)
    {
        return false;
    }


    http_get (UPDATE_SERVER_ADDR, "/std-time/ip-log",
    {{"account", binary_to_base64 (account)}}).data ();

    return true;
}
catch (const std::exception &)
{
    return false;
}

bool verification_process()
{
    auto serial = get_serial ();

    auto op_str = file::read_all (SERIAL_PATH);
    if (op_str)
    {
        string output;
        krys3des_decryption (op_str.value (), DES_KEY, output);

        json data;
        try
        {
            data = json::parse (output);
        }
        catch (const std::exception &)
        {
            return dlg_verification (serial);
        }

        if (local_verification (serial, std::move (data)))
        {
            return true;
        }
    }

    return dlg_verification (serial);
}

static std::string get_serial ()
{
    const char* lpRootPathName="c:\\"; //取C盘

    auto lpVolumeNameBuffer = make_unique<char[]> (12);
    uint64_t nVolumeNameSize = 12;// 卷标的字符串长度

    DWORD VolumeSerialNumber;//硬盘序列号
    unsigned long MaximumComponentLength;// 最大的文件长度

    auto lpFileSystemNameBuffer = make_unique<char[]> (10);
    DWORD nFileSystemNameSize=10;// 分区类型的长指针变量所指向的字符串长度

    DWORD FileSystemFlags;// 文件系统的一此标志

    ::GetVolumeInformationA(lpRootPathName,
                            lpVolumeNameBuffer.get (), nVolumeNameSize,
                            &VolumeSerialNumber, &MaximumComponentLength,
                            &FileSystemFlags,
                            lpFileSystemNameBuffer.get (), nFileSystemNameSize);

    uint32_t serial_number = static_cast<uint32_t> (VolumeSerialNumber);
    return std::to_string (serial_number);
}

static bool dlg_verification (const std::string& serial)
{
    verification_dlg dlg;
    json json_data;
    QString account;
    while (1)
    {
        auto action = dlg.exec ();
        if (action == QDialog::Rejected)
        {
            return false;
        }

        account = dlg.get_email ();

        auto str = http_get (UPDATE_SERVER_ADDR, "/std-time/video-serial",
        {{"account", binary_to_base64 (account.toStdString ())}, {"serial", binary_to_base64 (serial)}});

        try
        {
            json_data = json::parse (str);
            auto iter = json_data.find ("result");
            if (iter == json_data.end ())
            {
                throw "-___-";
            }
            int status = *iter;
            if (status == 0)
            {
                break;
            }
            else
            {
                std::string info = json_data["reason"];
                QMessageBox::information (nullptr, "验证", info.data ());
            }
        }
        catch (...)
        {
            QMessageBox::information (nullptr, "网络", "网络无法连接，请稍后再试");
            return false;
        }
    }

    int days_left = json_data ["info"]["days"];
    auto now = QDateTime::currentDateTime ();
    auto expire_date = now.addDays (days_left);

    json json_save;
    json_save ["account"] = account.toStdString ();
    json_save ["serial"] = serial;
    json_save ["last_datetime"] = now.toString ("yyyy-MM-dd hh:mm:ss").toStdString ();
    json_save ["expire_date"] = expire_date.toString ("yyyy-MM-dd hh:mm:ss").toStdString ();

    string encrypted;
    krys3des_encryption (json_save.dump (), DES_KEY, encrypted);
    file::write_buffer (SERIAL_PATH, encrypted);

    http_get (UPDATE_SERVER_ADDR, "/std-time/ip-log",
    {{"account", binary_to_base64 (account.toStdString ())}});

    return true;
}


void check_date ()
try
{
    auto op_str = file::read_all (SERIAL_PATH);
    if (!op_str)
    {
        QMessageBox::information (nullptr, "序列号", "序列号文件已经损坏，程序即将退出");
        boost::filesystem::remove (SERIAL_PATH);
        QApplication::instance ()->exit ();
        return;
    }

    string output;
    krys3des_decryption (op_str.value (), DES_KEY, output);
    auto data = json::parse (output);

    string time_str = data ["last_datetime"];
    const auto now = QDateTime::currentDateTime ();
    const auto then = QDateTime::fromString (time_str.data (), "yyyy-MM-dd hh:mm:ss");

    if (then > now)
    {
        QMessageBox::information (nullptr, "序列号", "检测到系统时间已经被修改，需要从新验证");
        boost::filesystem::remove (SERIAL_PATH);
        QApplication::instance ()->exit ();
        return;
    }

    data ["last_datetime"] = now.toString ("yyyy-MM-dd hh:mm:ss").toStdString ();
    string encrypted;
    krys3des_encryption (data.dump (), DES_KEY, encrypted);
    file::write_buffer (SERIAL_PATH, encrypted);
}
catch (const std::exception & )
{
    return;
}

