#include "registry.hpp"

#include "FN_types.hpp"
#include "FN_functions.hpp"

#include "RNA_access.h"

namespace FN { namespace DataFlowNodes {

	static void load_float(PointerRNA *ptr, Tuple &tuple, uint index)
	{
		float value = RNA_float_get(ptr, "value");
		tuple.set<float>(index, value);
	}

	static void load_vector(PointerRNA *ptr, Tuple &tuple, uint index)
	{
		float vector[3];
		RNA_float_get_array(ptr, "value", vector);
		tuple.set<Types::Vector>(index, Types::Vector(vector));
	}

	static void load_integer(PointerRNA *ptr, Tuple &tuple, uint index)
	{
		int value = RNA_int_get(ptr, "value");
		tuple.set<int32_t>(index, value);
	}

	static void load_float_list(PointerRNA *UNUSED(ptr), Tuple &tuple, uint index)
	{
		auto list = Types::SharedFloatList::New();
		tuple.move_in(index, list);
	}

	void initialize_socket_inserters(GraphInserters &inserters)
	{
		inserters.reg_socket_loader("fn_FloatSocket", load_float);
		inserters.reg_socket_loader("fn_VectorSocket", load_vector);
		inserters.reg_socket_loader("fn_IntegerSocket", load_integer);
		inserters.reg_socket_loader("fn_FloatListSocket", load_float_list);
	}

} }