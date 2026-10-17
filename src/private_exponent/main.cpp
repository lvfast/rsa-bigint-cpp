#include <iostream>  // Cho cout, cerr, endl, ios::sync_with_stdio, cin.tie
#include <fstream>   // Cho ifstream, ofstream
#include <string>    // Cho std::string, getline
#include <vector>    // Cho std::vector
#include <algorithm> // Cho std::reverse, std::max
#include <cstdint>   // Cho uint64_t, uint32_t, __uint128_t
#include <cmath>
#include <sstream>
#include <iomanip>

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

    // Nhân 2 số 64-bit trả về (low, high)
    static void mul64(uint64_t a, uint64_t b, uint64_t &low, uint64_t &high)
    {
        const uint64_t MASK32 = 0xFFFFFFFFULL;
        uint64_t a_lo = a & MASK32;
        uint64_t a_hi = a >> 32;
        uint64_t b_lo = b & MASK32;
        uint64_t b_hi = b >> 32;

        uint64_t lo_lo = a_lo * b_lo;
        uint64_t hi_lo = a_hi * b_lo;
        uint64_t lo_hi = a_lo * b_hi;
        uint64_t hi_hi = a_hi * b_hi;

        uint64_t carry = ((lo_lo >> 32) + (hi_lo & MASK32) + (lo_hi & MASK32)) >> 32;
        uint64_t mid = (hi_lo >> 32) + (lo_hi >> 32) + carry;
        high = hi_hi + mid;
        low = lo_lo + ((hi_lo & MASK32) << 32) + ((lo_hi & MASK32) << 32);
    }

    /*** Nhân hai BigInt (base 2^64) ***/
    static Big mul(const Big &a, const Big &b)
    {
        Big r;
        r.v.assign(a.v.size() + b.v.size(), 0);

        for (size_t i = 0; i < a.v.size(); ++i)
        {
            uint64_t carry = 0;
            for (size_t j = 0; j < b.v.size(); ++j)
            {
                uint64_t low, high;
                mul64(a.v[i], b.v[j], low, high); // gọi trực tiếp static mul64

                uint64_t sum = r.v[i + j] + low + carry;
                carry = (sum < low || sum < r.v[i + j]) ? 1 : 0;
                r.v[i + j] = sum;

                uint64_t next = r.v[i + j + 1] + high + carry;
                carry = (next < high) ? 1 : 0;
                r.v[i + j + 1] = next;
            }

            size_t pos = i + b.v.size();
            while (carry)
            {
                if (pos == r.v.size())
                    r.v.push_back(0);
                uint64_t sum = r.v[pos] + carry;
                carry = (sum < r.v[pos]) ? 1 : 0;
                r.v[pos++] = sum;
            }
        }

        r.normalize();
        return r;
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

    static void divmod(const Big &a, const Big &b, Big &q, Big &r)
    {
        if (b.isZero())
            throw std::runtime_error("Division by zero");

        // Nếu a < b thì q = 0, r = a
        if (cmp(a, b) < 0)
        {
            q = Big(0);
            r = a;
            return;
        }

        q.v.assign(a.v.size(), 0);
        r = Big(0);

        // Duyệt từng limb từ cao xuống thấp
        for (int i = (int)a.v.size() - 1; i >= 0; --i)
        {
            // r = (r << 64) + a.v[i]
            if (!r.v.empty())
                r.v.insert(r.v.begin(), 0);
            if (r.v.empty())
                r.v.push_back(a.v[i]);
            else
                r.v[0] = a.v[i];
            r.normalize();

            // Nếu r < b thì q limb = 0
            if (cmp(r, b) < 0)
            {
                q.v[i] = 0;
                continue;
            }

            // Binary search để tìm t sao cho b*t <= r < b*(t+1)
            uint64_t low = 0, high = 0xFFFFFFFFFFFFFFFFULL, best = 0;
            while (low <= high)
            {
                uint64_t mid = low + ((high - low) >> 1);
                Big prod = mul(b, Big(mid));
                int cmpRes = cmp(prod, r);
                if (cmpRes <= 0)
                {
                    best = mid;
                    low = mid + 1;
                }
                else
                    high = mid - 1;
            }

            q.v[i] = best;
            if (best != 0)
            {
                Big prod = mul(b, Big(best));
                r = sub(r, prod);
            }
        }

        q.normalize();
        r.normalize();
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

/*** Chia Big cho số nhỏ (1 <= m < 2^32), trả về phần dư ***/
/*** Chia Big cho số nhỏ (1 <= m < 2^32), trả về phần dư ***/
uint32_t divSmall(Big &a, uint32_t m)
{
    uint64_t rem = 0;
    const uint64_t BASE32 = (1ULL << 32);

    for (int i = (int)a.v.size() - 1; i >= 0; --i)
    {
        // chia limb 64-bit bằng 2 lần chia 32-bit để không cần 128-bit
        uint64_t hi = a.v[i] >> 32;
        uint64_t lo = a.v[i] & 0xFFFFFFFFULL;

        // kết hợp với rem để tạo số lớn nhưng vẫn nằm trong 64-bit từng bước
        uint64_t part = rem;
        uint64_t qh = part / m;
        rem = part % m;

        part = rem * BASE32 + hi;
        uint64_t q1 = part / m;
        rem = part % m;

        part = rem * BASE32 + lo;
        uint64_t q0 = part / m;
        rem = part % m;

        a.v[i] = (q1 << 32) | q0;
    }
    a.normalize();
    return (uint32_t)rem;
}

Big computePhi(const Big &p, const Big &q)
{
    Big one(1);
    Big p1 = Big::sub(p, one);
    Big q1 = Big::sub(q, one);
    Big phi = Big::mul(p1, q1);
    return phi;
}

void printBigHexLE(const Big &x)
{
    if (x.v.empty())
    {
        cout << "0";
        return;
    }

    // 1️⃣ Xuất toàn bộ giá trị ở dạng big-endian hex (mỗi limb 16 ký tự)
    std::ostringstream oss;
    oss << std::uppercase << std::hex;
    oss.fill('0');

    for (int i = (int)x.v.size() - 1; i >= 0; --i)
    {
        oss.width(16);
        oss << x.v[i];
    }

    std::string hexBE = oss.str(); // ví dụ: "00000000000001BB"

    // 2️⃣ Đảo ngược chuỗi thành little-endian
    std::reverse(hexBE.begin(), hexBE.end()); // ví dụ: "BB10000000000000"

    // 3️⃣ Bỏ tất cả các '0' dư ở cuối little-endian (phần cuối chuỗi)
    while (hexBE.size() > 1 && hexBE.back() == '0')
        hexBE.pop_back();

    // ✅ 4️⃣ In ra chuỗi kết quả
    std::cout << hexBE;
}

void printBigDec(Big x)
{
    if (x.isZero())
    {
        cout << 0;
        return;
    }
    vector<int> digits;
    while (!x.isZero())
    {
        uint32_t r = divSmall(x, 10);
        digits.push_back(r);
    }
    for (int i = (int)digits.size() - 1; i >= 0; --i)
        cout << digits[i];
}

Big gcd(Big a, Big b)
{
    Big zero;

    // Trường hợp 1: Nếu a == b
    if (Big::cmp(a, b) == 0)
        return a;

    // Trường hợp 2: Nếu a == 0 thì gcd(a, b) = b
    if (a.isZero())
        return b;

    // Trường hợp 3: Nếu b == 0 thì gcd(a, b) = a
    if (b.isZero())
        return a;

    // Bước 1: Loại bỏ các yếu tố 2 từ a và b
    int shiftCount = 0;

    // Loại bỏ 2^k từ cả a và b
    while (a.isEven() && b.isEven())
    {
        a.shr1(); // a = a / 2
        b.shr1(); // b = b / 2
        shiftCount++;
    }

    // Bước 2: Loại bỏ yếu tố 2 từ a hoặc b
    while (!a.isZero())
    {
        // Loại bỏ yếu tố 2 từ a
        while (a.isEven())
            a.shr1(); // a = a / 2

        // Loại bỏ yếu tố 2 từ b nếu b là số chẵn
        while (b.isEven())
            b.shr1(); // b = b / 2

        // Bước 3: Giảm a và b
        if (Big::cmp(a, b) >= 0)
            a = Big::sub(a, b); // a = a - b
        else
            b = Big::sub(b, a); // b = b - a
    }

    // Bước 4: Nhân lại các yếu tố 2 đã bị chia ở bước 1
    Big result = b;
    while (shiftCount > 0)
    {
        result.mul16(); // Nhân lại với 2^(shiftCount)
        shiftCount--;
    }

    return result;
}

Big modInverse(Big e, Big phi)
{
    Big zero(0), one(1);

    Big old_r = phi, r = e;
    Big old_s = zero, s = one;

    // Thuật toán Euclid mở rộng
    while (!r.isZero())
    {
        Big q, temp;
        Big::divmod(old_r, r, q, temp); // q = old_r / r, temp = old_r % r
        old_r = r;
        r = temp;

        Big tmp_s = s;
        // s = old_s - q * s
        Big qs = Big::mul(q, s);
        if (Big::cmp(old_s, qs) >= 0)
            s = Big::sub(old_s, qs);
        else
        {
            // Nếu âm → s = phi - ((qs - old_s) % phi)
            Big diff = Big::sub(qs, old_s);
            Big q2, modDiff;
            Big::divmod(diff, phi, q2, modDiff);
            s = Big::sub(phi, modDiff);
        }
        old_s = tmp_s;
    }

    // Sau vòng lặp: old_r = gcd(e, phi) = 1
    // old_s chính là nghịch đảo modular, nhưng có thể âm → chuẩn hóa về [0, phi)
    Big result = old_s;
    Big qtmp, rem;
    Big::divmod(result, phi, qtmp, rem);

    // Nếu rem âm → cộng thêm phi
    if (Big::cmp(rem, zero) < 0)
        rem = Big::add(rem, phi);

    return rem;
}

int main(int argc, char **argv)
{
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    if (argc < 2)
    {
        cerr << "Usage: " << argv[0] << " <input.inp>\n";
        return 1;
    }

    string inputFile = argv[1];
    string outputFile = inputFile.substr(0, inputFile.find_last_of('.')) + ".out";

    ifstream fin(inputFile);
    if (!fin)
    {
        cerr << "Cannot open input file\n";
        return 1;
    }

    string sp, sq, se;
    getline(fin, sp);
    getline(fin, sq);
    getline(fin, se);
    fin.close();

    // Parse hex little-endian thành BigInt
    Big p = parse_hex_le(sp);
    Big q = parse_hex_le(sq);
    Big e = parse_hex_le(se);

    // Tính phi(N) = (p - 1)*(q - 1)
    Big phi = computePhi(p, q);

    // Tính gcd(e, phi)
    Big g = gcd(e, phi);

    // Nếu gcd != 1 → không có d hợp lệ
    Big one(1);
    if (Big::cmp(g, one) != 0)
    {
        ofstream fout(outputFile);
        fout << "-1\n";
        fout.close();
        return 0;
    }

    // Tính d = e^-1 mod phi
    Big d = modInverse(e, phi);

    // Ghi d ra file ở dạng hex little-endian
    ofstream fout(outputFile);
    if (!fout)
    {
        cerr << "Cannot create output file\n";
        return 1;
    }

    std::ostringstream oss;
    std::streambuf *oldBuf = cout.rdbuf(oss.rdbuf()); // tạm redirect cout
    printBigHexLE(d);
    cout.rdbuf(oldBuf); // khôi phục cout

    fout << oss.str() << "\n";
    fout.close();

    return 0;
}
