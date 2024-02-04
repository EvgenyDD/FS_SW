#ifndef HELPER_H
#define HELPER_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <atomic>
#include <fstream>
#include <bitset>
#include <algorithm>
#include <math.h>
#include <numeric>
#include <stdio.h>
#include <iostream>
#include <stdarg.h>

class Helper
{
public:
    /// Vertextet einen Ganzzahlwert val im Hexadezimalformat.
    /// Auf die Minimal-Breite width wird mit führenden Nullen aufgefüllt;
    /// falls nicht angegeben, wird diese Breite aus dem Typ des Arguments
    /// abgeleitet. Funktion geeignet von char bis long long.
    /// Zeiger, Fließkommazahlen u.ä. werden nicht unterstützt, ihre
    /// Übergabe führt zu einem (beabsichtigten!) Compilerfehler.
    /// Grundlagen aus: http://stackoverflow.com/a/5100745/2932052
    template <typename T>
    static inline std::string int_to_hex(T val, size_t width=sizeof(T)*2)
    {
        std::stringstream ss;
        ss << std::setfill('0') << std::setw(width) << std::hex << (val|0);
        return ss.str();
    }

    template <typename T>
    static std::string int_to_bin(T val)
    {
        return std::bitset<8 * sizeof(T)>(val).to_string();
    }

    template <typename T>
    static std::string vec_to_string_dec(const T &v)
    {
        std::string s;
        for(const auto &i:v) { s += std::to_string(i) + " "; }
        return s;
    }

    template <typename T>
    static std::string vec_to_string_hex(const T &v)
    {
        std::string s;
        for(const auto &i:v) { s += int_to_hex(i) + " "; }
        return s;
    }

    template <typename T>
    static std::string vec_to_string_bin(const T &v)
    {
        std::string s;
        for(const auto &i:v) { s += int_to_bin(i) + " "; }
        return s;
    }

    template <typename T>
    static double avg(const T &v)
    {
        double sum = std::accumulate(v.begin(), v.end(), 0.0);
        return sum / v.size();
    }

    template <typename T>
    static double std_dev(const T &v)
    {
        double mean = avg(v);

        double sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
        return std::sqrt(sq_sum / v.size() - mean * mean);
    }

    template <typename T>
    static int sign(T val)
    {
        return (T(0) < val) - (val < T(0));
    }
};

class AnyContainer
{
public:
    AnyContainer(){}
    AnyContainer(const std::vector<uint8_t> &array) { data = array; }
    AnyContainer(const uint8_t *array, size_t size) { data.assign(array, array + size); }

    template<class T>
    void append(T n)
    {
        data.resize(data.size() + sizeof(T));
        memcpy(data.data() + data.size() - sizeof(T), &n, sizeof(T));
    }

    template<class T>
    T detach()
    {
        if(data.size() < sizeof(T)) throw std::string("Detach failed: array is too small " + std::to_string(data.size()));
        T t;
        memcpy(&t, data.data(), sizeof(T));
        data.erase(data.begin(), data.begin() + sizeof(T));
        return t;
    }

    void clear() { data.clear(); }

    size_t size() const { return data.size(); }

public:
    std::vector<uint8_t> data;
};

class AnyFile
{
public:
    AnyFile(std::string path) : path(path) {}
    ~AnyFile()
    {
        close();
    }

    bool open_r(bool is_binary_mode = false) { return open(std::fstream::in | (is_binary_mode ? std::fstream::binary : static_cast<std::ios_base::openmode>(0))); }
    bool open_w(bool is_binary_mode = false) { return open(std::fstream::out | (is_binary_mode ? std::fstream::binary : static_cast<std::ios_base::openmode>(0))); }
    bool open_append(bool is_binary_mode = false) { return open(std::fstream::in | std::fstream::out | std::fstream::app | (is_binary_mode ? std::fstream::binary : static_cast<std::ios_base::openmode>(0))); }

    template <class T>
    bool append(T t)
    {
        fs << t;
        if(fs.fail())
        {
            error = "Failed to write data to the file: " + path;
            return true;
        }
        return false;
    }

    std::vector<uint8_t> read()
    {
        return std::vector<uint8_t>{std::istreambuf_iterator<char>(fs), {}};
    }

    const auto &get_error() const { return error; }

    template <class... Args>
    static int write_to_file(std::string filename, size_t size, Args... args)
    {
        AnyFile file(filename);
        if(file.open_w())
        {
            std::cout << "[ERROR]\t" << file.get_error() << std::endl;
            return 1;
        }
        for(uint32_t i = 0; i < size; i++)
        {
            std::string sample;

            auto push = [&](auto x) { sample += std::to_string(x) + ", "; };
            push(i);
            (push(args.at(i)), ...);

            file.append(sample + "\n");
        }
        return 0;
    }

    template <class... Args>
    static int append_to_file(std::string filename, Args... args)
    {
        AnyFile file(filename);
        if(file.open_append())
        {
            std::cout << "[ERROR]\t" << file.get_error() << std::endl;
            return 1;
        }

        std::string sample;

        auto push = [&](auto x) { sample += std::to_string(x) + ", "; };
        (push(args), ...);

        file.append(sample + "\n");
        return 0;
    }

private:
    const std::string path;
    std::fstream fs;
    std::string error{"No error"};

    std::atomic_bool opened{false};

    void close()
    {
        if(opened)
        {
            fs.close();
        }
    }

    bool open(std::fstream::ios_base::openmode mode)
    {
        // TODO: add creation of the directories with
        // https://en.cppreference.com/w/cpp/filesystem/exists
        // https://en.cppreference.com/w/cpp/filesystem/path
        // https://en.cppreference.com/w/cpp/filesystem/create_directory
        fs.open(path, mode);
        if(fs.fail())
        {
            error = "Failed to open file: " + path;
            return true;
        }
        opened = true;
        return false;
    }
};

class StringHelper
{
public:
    static std::vector<std::string> split(std::string s, char separator)
    {
        std::istringstream f(s);
        std::vector<std::string> strings;
        std::string temp;
        while(getline(f, temp, separator))
        {
            strings.push_back(temp);
        }
        if(strings.empty()) strings.push_back("");
        return strings;
    }

    static bool contains(const std::string &s, std::string match)
    {
        return s.find(match) != std::string::npos;
    }

    static bool contains(const std::vector<std::string> &s, std::string match)
    {
        for(const auto &i : s) {if(i == match) return true;}
        return false;
    }

    static int count(const std::string &s, char c)
    {
        return std::count(s.begin(), s.end(), c);
    }

    static std::string remove(std::string s, char symbol)
    {
        s.erase(std::remove(s.begin(), s.end(), symbol), s.end());
        return s;
    }

    static std::string printf(const std::string fmt, ...)
    {
        auto size = fmt.size() * 2 + 50;   // Use a rubric appropriate for your code
        std::string str;
        va_list ap;
        while(1)  // Maximum two passes on a POSIX system...
        {
            str.resize(size);
            va_start(ap, fmt);
            int n = vsnprintf(static_cast<char *>(str.data()), size, fmt.c_str(), ap);
            va_end(ap);
            if (n > -1 && n < static_cast<int>(size)) // Everything worked
            {
                str.resize(static_cast<size_t>(n));
                return str;
            }
            if (n > -1)  // Needed size returned
                size = static_cast<unsigned int>(n + 1);   // For null char
            else
                size *= 2;      // Guess at a larger size (OS specific)
        }
    }
};

class VectorHelper
{
public:
    template<class A, class B>
    static std::vector<A> convert_array(const std::vector<B> &b)
    {
        std::vector<A> r;
        for(const auto &i : b)
        {
            A e = i;
            r.push_back(e);
        }
        return r;
    }

    template<class A>
    static void append(A &out, const A &v)
    {
        out.insert(out.end(), v.begin(), v.end());
    }

    template<typename T>
    static std::vector<T> differentiate(const std::vector<T> &v)
    {
        std::vector<T> diff;
        diff.resize(v.size());
        for(uint32_t i=1; i<v.size(); i++) diff[i-1] = v[i] - v[i-1];
        diff[diff.size() - 1] = diff[diff.size() - 2]; // hack
        return diff;
    }

    template<typename T>
    static std::vector<T> differentiate(const std::vector<T> &v, T step, int size = 0)
    {
        if(size == 0) size = v.size();
        std::vector<T> diff;
        diff.resize(v.size());
        for(int i=1; i<size; i++) diff[i-1] = (v[i] - v[i-1]) / step;

        int i_start = size - 2;
        T end = diff[i_start];
        for(int i=i_start + 1; i<v.size(); i++)
        {
            diff[i] = end;
        }
        return diff;
    }
};

class MatrixHelper
{
public:
    template<typename T>
    static std::vector<std::vector<T>> inverse(std::vector<std::vector<T>> m)
    {
        if(m.size() != m[0].size()) throw std::string("WTF man!");

        int n = m.size();
        for(auto &i:m) { i.resize(n*2); }

        for(int i = 0; i < n; i++)
        {
            for(int j = 0; j < 2 * n; j++)
            {
                if(j == (i + n)) { m[i][j] = 1; }
            }
        }

        for(int i = n - 1; i > 0; i--)
        {
            if(m[i - 1][0] < m[i][0])
            {
                auto temp = m[i];
                m[i] = m[i - 1];
                m[i - 1] = temp;
            }
        }

        for(int i = 0; i < n; i++)
        {
            for(int j = 0; j < n; j++)
            {
                if(j != i)
                {
                    auto temp = m[j][i] / m[i][i];
                    for(int k = 0; k < 2 * n; k++) { m[j][k] -= m[i][k] * temp; }
                }
            }
        }

        for(int i = 0; i < n; i++)
        {
            auto temp = m[i][i];
            for (int j = 0; j < 2 * n; j++)
            {

                m[i][j] = m[i][j] / temp;
            }
        }

        std::vector<std::vector<T>> out;
        out.resize(n);
        for(int i=0; i<n; i++)
        {
            for(int j=0; j<n; j++)
            {
                out[i].push_back(m[i][j+n]);
            }
        }

        return out;
    }
};

#endif // HELPER_H
