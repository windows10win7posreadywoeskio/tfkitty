#pragma once
// minified source sdk
namespace ssdk {
	struct SendTable;

	struct SendProp {
		typedef enum
		{
			DPT_Int = 0,
			DPT_Float,
			DPT_Vector,
			DPT_String,
			DPT_Array,	// An array of the base types (can't be of datatables).
			DPT_DataTable,
#if 0 // We can't ship this since it changes the size of DTVariant to be 20 bytes instead of 16 and that breaks MODs!!!
			DPT_Quaternion,
#endif
			DPT_NUMSendPropTypes
		} SendPropType;

		void* vtable; // FUCK!!!!

		void* m_pMatchingRecvProp;	// This is temporary and only used while precalculating
		// data for the decoders.

		SendPropType	m_Type;
		int				m_nBits;
		float			m_fLowValue;
		float			m_fHighValue;

		SendProp* m_pArrayProp;					// If this is an array, this is the property that defines each array element.
		void* m_ArrayLengthProxy;	// This callback returns the array length.

		int				m_nElements;		// Number of elements in the array (or 1 if it's not an array).
		int				m_ElementStride;	// Pointer distance between array elements.

		union
		{
			char* m_pExcludeDTName;			// If this is an exclude prop, then this is the name of the datatable to exclude a prop from.
			char* m_pParentArrayPropName;
		};

		char* m_pVarName;
		float			m_fHighLowMul;


		int					m_Flags;				// SPROP_ flags.

		void* m_ProxyFn;				// NULL for DPT_DataTable.
		void* m_DataTableProxyFn;		// Valid for DPT_DataTable.

		SendTable* m_pDataTable;

		// SENDPROP_VECTORELEM makes this negative to start with so we can detect that and
		// set the SPROP_IS_VECTOR_ELEM flag.
		int					m_Offset;

		// Extra data bound to this property.
		const void* m_pExtraData;
	};

	static_assert(sizeof(SendProp) == 76);

	struct SendTable {
		SendProp* m_pProps;
		int			m_nProps;
		char* m_pNetTableName;	// The name matched between client and server.
	};

	struct ServerClass {
		char* m_pNetworkName;
		SendTable* m_pTable;
		ServerClass* m_pNext;
	};
}
