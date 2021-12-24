#ifndef DRW_DBG_BASE_H
#define DRW_DBG_BASE_H

#include <iostream>
#include <iomanip>

namespace DRW
{
    enum class DebugLevel
    {
        None,
        Debug
    };

/**
 * Interface for debug printers.
 *
 * The base class is silent and ignores all debugging.
 */
    class IDebugPrinter
    {
    public:
        IDebugPrinter() = default;
        virtual ~IDebugPrinter() = default;
        virtual void printString(const std::string &s) const = 0;
        virtual void printInt(long long int i) const = 0;
        virtual void printUnsignedInt(long long unsigned int i) const = 0;
        virtual void printDouble(double d) const = 0;
        virtual void printHex(long long int i) const= 0;
        virtual void printBinary(int i) const = 0;
        virtual void printHL(int c, int s, int h) const = 0;
        virtual void printPoint(double x, double y, double z) const = 0;
    };

    class SilentPrinter : public IDebugPrinter
    {
    public:
        SilentPrinter() = default;
        ~SilentPrinter() override = default;

        void printString(const std::string &s) const override { (void) s; }

        void printInt(long long int i) const override { (void) i; }

        void printUnsignedInt(long long unsigned int i) const override { (void) i; }

        void printDouble(double d) const override { (void) d; }

        void printHex(long long int i) const override { (void) i; }

        void printBinary(int i) const override { (void) i; }

        void printHL(int c, int s, int h) const override {
            (void) c;
            (void) s;
            (void) h;
        }

        void printPoint(double x, double y, double z) const override {
            (void) x;
            (void) y;
            (void) z;
        }
    };

    class CErrPrinter : public IDebugPrinter
    {
    public:
        void printString(const std::string &s) const override { std::cerr << s; }

        void printInt(long long int i) const override { std::cerr << i; }

        void printUnsignedInt(long long unsigned int i) const override { std::cerr << i; }

        void printDouble(double d) const override { std::cerr << std::fixed << d; }

        void printHex(long long int i) const override {
            std::cerr << "0x" << std::setw(2) << std::setfill('0');
            std::cerr << std::hex << i;
            std::cerr.flags(flags);
        }

        void printBinary(int i) const override {
            std::cerr << std::setw(8) << std::setfill('0');
            std::cerr << std::setbase(2) << i;
            std::cerr.flags(flags);
        }

        void printHL(int c, int s, int h) const override {
            std::cerr << c << '.' << s << '.';
            std::cerr << "0x" << std::setw(2) << std::setfill('0');
            std::cerr << std::hex << h;
            std::cerr.flags(flags);
        }

        void printPoint(double x, double y, double z) const override {
            std::cerr << std::fixed << "x: " << x << ", y: " << y << ", z: " << z;
        }

    private:
        std::ios_base::fmtflags flags{std::cerr.flags()};
    };
}
#endif