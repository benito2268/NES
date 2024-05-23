#pragma once
#include <vector>
#include <string>

class cDataBus;

inline const std::string RESULTS[2] = {"FAILED", "PASSED"};

class Tester {
private:
	cDataBus *bus;
	bool test(std::string name, bool (Tester::*fun)());

public:
	~Tester() { delete bus; }

	bool run_tests();

	//addressing modes
	bool test_imm();
	bool test_zp();
	bool test_abs();
	bool test_rel();

	bool test_ind();


	//instructions
	bool test_adc();
	bool test_sbc();
};

