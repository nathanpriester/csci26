#include <iostream>
#include <fstream>
#include <string>
using namespace std;
void convert_decimal(int base, int input, string& result){
	if (input == 0) return;
	int a = input / base;
	int remainder = input % base;
	string remainder_str = to_string(remainder);
	if (base == 16){
		if (remainder == 10) remainder_str = 'A';
		if (remainder == 11) remainder_str = 'B';
		if (remainder == 12) remainder_str = 'C';
		if (remainder == 13) remainder_str = 'D';
		if (remainder == 14) remainder_str = 'E';
		if (remainder == 15) remainder_str = 'F';
	}
	result.insert(0,remainder_str);
	convert_decimal(base,a,result);
	return;
}
void string_pad(string& input, int pad){
	while(input.length() < pad) input.insert(0,"0");	
}
int main() {
	unsigned char ch;
	unsigned int binary = 2, octal = 8, decimal = 10, hex = 16, count = 1;
	string count_output, octal_output, hex_output, binary_output, ascii_output;
	while(cin){
		int peek = cin.peek();
		if(peek == EOF) break;
		cin >> noskipws >> ch;
		int decimal_int = ch;
		string octal_byte, hex_byte, binary_byte;
		//------------------------------convert to binary, octal, hex, and then pad
		convert_decimal(octal, decimal_int, octal_byte);
		string_pad(octal_byte,3);
		convert_decimal(hex, decimal_int, hex_byte);
		string_pad(hex_byte,2);
		convert_decimal(binary, decimal_int, binary_byte);
		string_pad(binary_byte,8);
		//-----------------------------build the 4 byte strings
		octal_output += octal_byte;
		hex_output += hex_byte;
		binary_output += binary_byte;
		if(isprint(ch)) ascii_output += ch;
		else ascii_output += ".";
		//-----------------------------------every 4 bytes cout strtings, then clear them
		if (count % 4 == 0) {
			convert_decimal(hex, count-4, count_output);
			if (count_output == "") count_output = "0";
			string_pad(count_output,5);
			cout<<count_output<<"\t"<<binary_output<<"\t"<<octal_output<<"\t"<<hex_output<<"\t"<<ascii_output<<endl;
			count_output = binary_output = octal_output = hex_output = ascii_output = "";
		}
		count++;
	}
	return 0;
}
