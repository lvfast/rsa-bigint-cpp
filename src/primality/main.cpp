#include <iostream>  // Cho cout, cerr, endl, ios::sync_with_stdio, cin.tie
#include <fstream>   // Cho ifstream, ofstream
#include <string>    // Cho std::string, getline
#include <vector>    // Cho std::vector
#include <algorithm> // Cho std::reverse, std::max
#include <cstdint>   // Cho uint64_t, uint32_t, __uint128_t
using namespace std;

/***  BigInt  ***/
struct Big
{
    /*
    mỗi v[i] là 1 limb theo base 2^64
    ví dụ:
    2^64: v[0] = 0, v[1] = 1. => 0x(2^64)^0 + 1x(2^64)^1
    */
    vector<uint64_t> v;

    Big() {}
    explicit Big(uint64_t x)
    {
        if (x)
            v.push_back(x);
    }

    bool isZero() const { return v.empty(); }
    bool isEven() const { return isZero() || ((v[0] & 1ULL) == 0ULL); }

    void normalize()
    {
        while (!v.empty() && v.back() == 0ULL)
            v.pop_back();
    }

    // đi từ cuối mảng về đầu (vì lớn nhất ở cuối)
    // nếu limb của thằng nào lớn hơn thằng kia thì thằng đó lớn hơn
    static int cmp(const Big &a, const Big &b)
    {
        if (a.v.size() != b.v.size())
            return (a.v.size() < b.v.size()) ? -1 : 1;
        for (int i = (int)a.v.size() - 1; i >= 0; --i)
        {
            if (a.v[i] != b.v[i])
                return (a.v[i] < b.v[i]) ? -1 : 1;
        }
        return 0;
    }

    // a + b
    static Big add(const Big &a, const Big &b)
    {
        Big r;
        r.v.resize(max(a.v.size(), b.v.size()) + 1, 0);
        uint64_t carry = 0;

        for (size_t i = 0; i < r.v.size(); ++i)
        {
            uint64_t ai = (i < a.v.size() ? a.v[i] : 0ULL);
            uint64_t bi = (i < b.v.size() ? b.v[i] : 0ULL);

            // Thực hiện cộng thủ công 64-bit có carry
            uint64_t sum = ai + bi;
            uint64_t newCarry = (sum < ai) ? 1 : 0; // nếu tràn thì carry = 1

            sum += carry;
            if (carry && sum == 0)
                newCarry = 1; // nếu cộng carry gây tràn thêm 1 lần nữa

            r.v[i] = sum;
            carry = newCarry;
        }

        r.normalize();
        return r;
    }

    // a += small (0..15)
    void addSmall(uint32_t d)
    {
        uint64_t carry = d;
        size_t i = 0;

        while (carry)
        {
            if (i == v.size())
                v.push_back(0);

            uint64_t prev = v[i];
            uint64_t sum = prev + carry;

            // Kiểm tra tràn 64-bit
            carry = (sum < prev) ? 1 : 0;

            v[i] = sum;
            ++i;
        }
    }

    // r = a - b, requires a >= b
    static Big sub(const Big &a, const Big &b)
    {
        Big r;
        r.v.resize(a.v.size(), 0);
        uint64_t borrow = 0;

        for (size_t i = 0; i < a.v.size(); ++i)
        {
            uint64_t ai = a.v[i];
            uint64_t bi = (i < b.v.size() ? b.v[i] : 0ULL);
            uint64_t tmp = bi + borrow;

            // Nếu ai < bi + borrow → cần mượn
            if (ai < tmp)
            {
                r.v[i] = (uint64_t)((~0ULL) - (tmp - ai - 1)); // cộng thêm 2^64
                borrow = 1;
            }
            else
            {
                r.v[i] = ai - tmp;
                borrow = 0;
            }
        }

        r.normalize();
        return r;
    }

    // shift left by 4 bits (multiply by 16)
    void mul16()
    {
        if (v.empty())
            return;
        uint64_t carry = 0;
        for (size_t i = 0; i < v.size(); ++i)
        {
            uint64_t nv = (v[i] << 4) | carry;
            carry = (v[i] >> 60);
            v[i] = nv;
        }
        if (carry)
            v.push_back(carry);
    }

    // shift right by 1 bit (divide by 2, floor)
    void shr1()
    {
        uint64_t carry = 0;
        for (int i = (int)v.size() - 1; i >= 0; --i)
        {
            uint64_t newCarry = v[i] & 1ULL;
            v[i] = (v[i] >> 1) | (carry << 63);
            carry = newCarry;
        }
        normalize();
    }

    // compare to small (uint64)
    int cmpSmall(uint64_t x) const
    {
        if (v.size() > 1)
            return 1;
        uint64_t val = v.empty() ? 0ULL : v[0];
        if (val < x)
            return -1;
        if (val > x)
            return 1;
        return 0;
    }

    // set to n - 1 (assumes n > 0)
    static Big dec1(const Big &n)
    {
        Big r = n;
        if (r.v.empty())
            return r;
        size_t i = 0;
        while (true)
        {
            if (r.v[i] > 0)
            {
                r.v[i] -= 1;
                break;
            }
            r.v[i] = ~0ULL; // borrow
            ++i;
        }
        r.normalize();
        return r;
    }

    // res = (a + b) % mod, assuming 0 <= a,b < mod
    static Big addMod(const Big &a, const Big &b, const Big &mod)
    {
        Big s = add(a, b);
        if (cmp(s, mod) >= 0)
            s = sub(s, mod);
        return s;
    }

    // res = (2*a) % mod
    static Big doubleMod(const Big &a, const Big &mod)
    {
        Big s = add(a, a);
        if (cmp(s, mod) >= 0)
            s = sub(s, mod);
        return s;
    }

    // res = (a * b) % mod using double-and-add (no big multiplication)
    static Big mulMod(Big a, Big b, const Big &mod)
    {
        Big res; // zero
        // ensure a,b < mod (in this design they should be)
        while (!b.isZero())
        {
            if (!b.isEven())
            {
                res = addMod(res, a, mod);
            }
            a = doubleMod(a, mod);
            b.shr1();
        }
        return res;
    }

    // res = (base^exp) % mod, exp is Big
    static Big powMod(Big base, Big exp, const Big &mod)
    {
        Big res(1);
        // reduce base if needed (not strictly necessary if base < mod by design)
        if (cmp(base, mod) >= 0)
        {
            // reduce by repeated subtraction (rare path). Use simple long subtraction ladder:
            // bring base into [0, mod)
            while (cmp(base, mod) >= 0)
            {
                // subtract the largest shift of mod <= base
                // find shift
                int shift = (int)base.v.size() - (int)mod.v.size();
                if (shift < 0)
                    break;
                // make tmp = mod << (shift*64)
                Big tmp;
                tmp.v.assign(shift, 0);
                tmp.v.insert(tmp.v.end(), mod.v.begin(), mod.v.end());
                if (cmp(base, tmp) < 0)
                { // step back one limb
                    --shift;
                    if (shift < 0)
                        break;
                    tmp.v.clear();
                    tmp.v.assign(shift, 0);
                    tmp.v.insert(tmp.v.end(), mod.v.begin(), mod.v.end());
                }
                base = sub(base, tmp);
            }
        }
        while (!exp.isZero())
        {
            if (!exp.isEven())
                res = mulMod(res, base, mod);
            base = mulMod(base, base, mod);
            exp.shr1();
        }
        return res;
    }
};

/*** Parse HEX (uppercase) little-endian string into Big ***/
Big parse_hex_le(const string &s_raw)
{
    // 1) trim spaces/newlines
    auto trim = [](string s)
    {
        size_t a = s.find_first_not_of(" \t\r\n");
        size_t b = s.find_last_not_of(" \t\r\n");
        if (a == string::npos)
            return string();
        return s.substr(a, b - a + 1);
    };
    string s = trim(s_raw);
    // 2) reverse (little-endian hex to big-endian hex)
    reverse(s.begin(), s.end());
    // 3) parse: value = value*16 + digit
    Big val; // 0
    auto hexVal = [](char c) -> int
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10; // just in case
        return -1;
    };
    for (char c : s)
    {
        int d = hexVal(c);
        if (d < 0)
            continue;              // ignore non-hex (shouldn't happen in tests)
        val.mul16();               // *16
        val.addSmall((uint32_t)d); // +digit
    }
    val.normalize();
    return val;
}

/*** Primality test using Miller-Rabin ***/
bool isPrime(const Big &n)
{
    // handle small cases
    if (n.isZero())
        return false;
    if (n.cmpSmall(2) < 0)
        return false; // n < 2
    if (n.cmpSmall(2) == 0)
        return true; // n == 2
    if (n.isEven())
        return false; // even > 2
    if (n.cmpSmall(3) == 0)
        return true; // n == 3

    // write n-1 = d * 2^s with d odd
    Big n1 = Big::dec1(n);
    Big d = n1;
    int s = 0;
    while (d.isEven())
    {
        d.shr1();
        ++s;
    }

    // fixed bases (12 bases). Good probabilistic security for up to 512-bit inputs in contest settings.
    static const uint32_t bases_arr[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37};
    for (uint32_t a_small : bases_arr)
    {
        // if a >= n-2, skip
        if (n.cmpSmall(a_small) <= 0)
            continue; // when n <= a
        // x = a^d mod n
        Big a(a_small);
        Big x = Big::powMod(a, d, n);
        // x == 1 or x == n-1 ?
        if (Big::cmp(x, Big(1)) == 0 || Big::cmp(x, n1) == 0)
            continue;

        bool witness = true;
        Big x2 = x;
        for (int r = 1; r < s; ++r)
        {
            x2 = Big::mulMod(x2, x2, n);
            if (Big::cmp(x2, n1) == 0)
            {
                witness = false;
                break;
            }
            if (Big::cmp(x2, Big(1)) == 0)
                return false; // non-trivial square root of 1 → composite
        }
        if (witness)
            return false; // strong witness found
    }
    return true; // probably prime
}

int main(int argc, char **argv)
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 3)
    {
        cerr << "Usage: " << argv[0] << " <input.inp> <output.out>\n";
        return 1;
    }
    string inPath = argv[1];
    string outPath = argv[2];

    ifstream fin(inPath);
    if (!fin)
    {
        cerr << "Cannot open input file\n";
        return 1;
    }
    string line;
    getline(fin, line);
    fin.close();

    Big n = parse_hex_le(line);
    bool prime = isPrime(n);

    ofstream fout(outPath);
    if (!fout)
    {
        cerr << "Cannot open output file\n";
        return 1;
    }
    fout << (prime ? '1' : '0') << "\n";
    fout.close();
    return 0;
}
