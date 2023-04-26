// Metacompiler code-gen
// clang-format off

#include "Module.h"
#include "TestSystem.h"
// System move_1_unit_x
static void _system_0_invoke(ecs_iter_t* it)
{
	TransformComponent* c0 = ecs_field(it, TransformComponent, 1);
	for (int i = 0; i < it->count; ++i)
	{
	flecs::entity entity(it->world, it->entities[i]);
		move_1_unit_x(
			entity,
			c0[i]);
	}
}
static SystemDescriptor _system_0_desc = {
	.name = "move_1_unit_x",
	.invoke = _system_0_invoke,
	.filter_desc = {
		.terms = {
			{
				.src = {
					.id = EcsThis,
					.name = nullptr,
					.trav = EcsIsA,
					.flags = 6, // (EcsSelf,EcsUp,)
				},
				.first = {
					.name = (char*)"TransformComponent",
				},
				.inout = EcsInOutDefault,
				.oper = EcsAnd,
			},
		},
	},
	.phase = EcsOnUpdate,
};

void import_module_Engine(SystemDescriptorRegistrar* reg)
{
	reg->add(&_system_0_desc);
}
// clang-format on
