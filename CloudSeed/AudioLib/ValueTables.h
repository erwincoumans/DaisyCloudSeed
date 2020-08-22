#ifndef AUDIOLIB_VALUETABLES
#define AUDIOLIB_VALUETABLES

#include "MathDefs.h"


namespace AudioLib
{
	class ValueTables
	{
	public:
		static const int TableSize = 40001;

		static DSY_SDRAM_BSS double Sqrt[TableSize];
		static DSY_SDRAM_BSS double Sqrt3[TableSize];
		static DSY_SDRAM_BSS double Pow1_5[TableSize];
		static DSY_SDRAM_BSS double Pow2[TableSize];
		static DSY_SDRAM_BSS double Pow3[TableSize];
		static DSY_SDRAM_BSS double Pow4[TableSize];
		static DSY_SDRAM_BSS double x2Pow3[TableSize];

		// octave response. value double every step (2,3,4,5 or 6 steps)
		static DSY_SDRAM_BSS double Response2Oct[TableSize];
		static DSY_SDRAM_BSS double Response3Oct[TableSize];
		static DSY_SDRAM_BSS double Response4Oct[TableSize];
		static DSY_SDRAM_BSS double Response5Oct[TableSize];
		static DSY_SDRAM_BSS double Response6Oct[TableSize];

		// decade response, value multiplies by 10 every step
		static DSY_SDRAM_BSS double Response2Dec[TableSize];
		static DSY_SDRAM_BSS double Response3Dec[TableSize];
		static DSY_SDRAM_BSS double Response4Dec[TableSize];

		static void Init();
		static double Get(double index, double* table);
	};
}

#endif
