#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <iomanip>
using namespace std;

/***  BigInt  ***/
struct Big
{
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

    static Big add(const Big &a, const Big &b)
    {
        Big r;
        r.v.resize(max(a.v.size(), b.v.size()) + 1, 0);
        uint64_t carry = 0;

        for (size_t i = 0; i < r.v.size(); ++i)
        {
            uint64_t ai = (i < a.v.size() ? a.v[i] : 0ULL);
            uint64_t bi = (i < b.v.size() ? b.v[i] : 0ULL);
            uint64_t sum = ai + bi;
            uint64_t newCarry = (sum < ai) ? 1 : 0;
            sum += carry;
            if (carry && sum == 0)
                newCarry = 1;
            r.v[i] = sum;
            carry = newCarry;
        }

        r.normalize();
        return r;
    }

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
            if (ai < tmp)
            {
                r.v[i] = (uint64_t)((~0ULL) - (tmp - ai - 1));
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

    static Big addMod(const Big &a, const Big &b, const Big &mod)
    {
        Big s = add(a, b);
        if (cmp(s, mod) >= 0)
            s = sub(s, mod);
        return s;
    }

    static Big doubleMod(const Big &a, const Big &mod)
    {
        Big s = add(a, a);
        if (cmp(s, mod) >= 0)
            s = sub(s, mod);
        return s;
    }

    static Big mulMod(Big a, Big b, const Big &mod)
    {
        Big res;
        while (!b.isZero())
        {
            if (!b.isEven())
                res = addMod(res, a, mod);
            a = doubleMod(a, mod);
            b.shr1();
        }
        return res;
    }

    static Big powMod(Big base, Big exp, const Big &mod)
    {
        Big res(1);
        while (!exp.isZero())
        {
            if (!exp.isEven())
                res = mulMod(res, base, mod);
            base = mulMod(base, base, mod);
            exp.shr1();
        }
        return res;
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
            carry = (sum < prev) ? 1 : 0;

            v[i] = sum;
            ++i;
        }
    }
};

/*** Parse HEX (uppercase) little-endian string into Big ***/
Big parse_hex_le(const string &s_raw)
{
    auto trim = [](string s)
    {
        size_t a = s.find_first_not_of(" \t\r\n");
        size_t b = s.find_last_not_of(" \t\r\n");
        if (a == string::npos)
            return string();
        return s.substr(a, b - a + 1);
    };
    string s = trim(s_raw);
    reverse(s.begin(), s.end());
    Big val;
    auto hexVal = [](char c) -> int
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        if (c >= 'A' && c <= 'F')
            return c - 'A' + 10;
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
        return -1;
    };
    for (char c : s)
    {
        int d = hexVal(c);
        if (d < 0)
            continue;
        val.mul16();
        val.addSmall((uint32_t)d);
    }
    val.normalize();
    return val;
}

/*** In kết quả ở dạng HEX little-endian ***/
void printBigHexLE(const Big &x)
{
    if (x.v.empty())
    {
        cout << "0";
        return;
    }

    ostringstream oss;
    oss << uppercase << hex;
    oss.fill('0');

    for (int i = (int)x.v.size() - 1; i >= 0; --i)
    {
        oss.width(16);
        oss << x.v[i];
    }

    string hexBE = oss.str();
    reverse(hexBE.begin(), hexBE.end());
    while (hexBE.size() > 1 && hexBE.back() == '0')
        hexBE.pop_back();
    cout << hexBE;
}

/*** MAIN: thực hiện y = x^k mod N ***/
int main(int argc, char **argv)
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 3)
    {
        cerr << "Usage: " << argv[0] << " <input.inp> <output.out>\n";
        return 1;
    }

    ifstream fin(argv[1]);
    if (!fin)
    {
        cerr << "Cannot open input file\n";
        return 1;
    }

    string sN, sk, sx;
    getline(fin, sN);
    getline(fin, sk);
    getline(fin, sx);
    fin.close();

    Big N = parse_hex_le(sN);
    Big k = parse_hex_le(sk);
    Big x = parse_hex_le(sx);

    Big y = Big::powMod(x, k, N);

    ofstream fout(argv[2]);
    if (!fout)
    {
        cerr << "Cannot create output file\n";
        return 1;
    }

    ostringstream oss;
    streambuf *oldBuf = cout.rdbuf(oss.rdbuf());
    printBigHexLE(y);
    cout.rdbuf(oldBuf);

    fout << oss.str() << "\n";
    fout.close();
    return 0;
}