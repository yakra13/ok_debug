#pragma once

#define OBF(str) ObfuscatedString(str)

constexpr unsigned char OBF_KEY = 0x55;

template <typename T, size_t N>
class ObfuscatedString
{
private:
    T encrypted[N];
    T decrypted[N];

public:
    consteval ObfuscatedString(const T (&str)[N])
        : encrypted{}, decrypted{}
    {
        for (size_t i = 0; i < N; i++)
        {
            encrypted[i] = str[i] ^ OBF_KEY;
        }
    }

    ~ObfuscatedString()
    {
        volatile T* ptr = decrypted;

        for (size_t i = 0; i < N; i++)
        {
            ptr[i] = 0;
        }
    }

    T* get()
    {

        for (size_t i = 0; i < N; i++)
        {
            decrypted[i] = encrypted[i] ^ OBF_KEY;
        }

        return decrypted;
    }
};
