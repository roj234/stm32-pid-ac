
// [20.1, 29.9]
const u16 ADC20[] = {1822,1827,1832,1836,1841,1845,1850,1855,1859,1864,1869,1873,1878,1883,1887,1892,1896,1901,1906,1910,1915,1920,1924,1929,1933,1938,1943,1947,1952,1956,1961,1966,1970,1975,1979,1984,1989,1993,1998,2002,2007,2012,2016,2021,2025,2030,2034,2039,2043,2048,2053,2057,2062,2066,2071,2075,2080,2084,2089,2093,2098,2102,2107,2111,2116,2120,2125,2129,2134,2138,2143,2147,2152,2156,2161,2165,2170,2174,2179,2183,2187,2192,2196,2201,2205,2210,2214,2218,2223,2227,2232,2236,2240,2245,2249,2253,2258,2262,2267};
// [-30, 50]
const u16 ADC50[] = {195,208,221,235,250,265,282,299,317,335,355,375,397,419,442,466,491,517,544,571,600,630,661,692,725,758,793,828,864,901,939,978,1017,1058,1098,1140,1182,1225,1269,1313,1357,1402,1448,1493,1539,1585,1632,1678,1725,1771,1818,1864,1910,1956,2002,2048,2093,2138,2183,2227,2271,2314,2357,2399,2441,2482,2522,2562,2601,2639,2677,2714,2750,2786,2821,2855,2888,2921,2953,2984,3014};

u8 binarySearch(const u16 a[], u16 high, u16 key) {
	u16 low = 0;

	while (low <= high) {
		u16 mid = (low + high) >> 1;
		int16_t midVal = a[mid] - key;

		if (midVal < 0) {
			low = mid + 1;
		} else if (midVal > 0) {
			high = mid - 1;
		} else {
			return mid;
		}
	}

	return low;
}

int16_t AdcToTemp(u16 adc) {
	if (adc < 1822 || adc > 2267) {
		return -300 + binarySearch(ADC50, sizeof(ADC50)-1, adc) * 10;
	} else {
		return 201 + binarySearch(ADC20, sizeof(ADC20)-1, adc);
	}
}
