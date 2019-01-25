#pragma once
#include "ui/Quantity.hpp"
#include "engine/Module.hpp"
#include "engine/Param.hpp"


namespace rack {


/** A Quantity that wraps an engine Param */
struct ParamQuantity : Quantity {
	Module *module = NULL;
	int paramId = 0;

	Param *getParam();
	/** Request to the engine to smoothly set the value */
	void setSmoothValue(float smoothValue);
	float getSmoothValue();

	void setValue(float value) override;
	float getValue() override;
	float getMinValue() override;
	float getMaxValue() override;
	float getDefaultValue() override;
	float getDisplayValue() override;
	void setDisplayValue(float displayValue) override;
	std::string getDisplayValueString() override;
	void setDisplayValueString(std::string s) override;
	int getDisplayPrecision() override;
	std::string getLabel() override;
	std::string getUnit() override;
};


} // namespace rack