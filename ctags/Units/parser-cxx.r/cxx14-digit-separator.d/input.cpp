long a1=1'048'576;
long a2=0x10'0000;
long a3=00'04'00'00'00;
long a4=0b100'000000'000000'000000;

void test()
{
	call(1'048'576);
	call(1'048'576,0x10'0000,'a',00'04'00'00'00);

	long b1=1'048'576;
	long c1 = 'a';
	long b2=0x10'0000;
	long c2 = 'a';
	long b3=00'04'00'00'00;
	long c3 = 'a';
	long b4=0b100'000000'000000'000000;
	long c4 = 'a';
}

class X
{
public:
	long m1=1'048'576;
	long m2=0x10'0000;
	long m3=00'04'00'00'00;
	long m4=0b100'000000'000000'000000;
};
