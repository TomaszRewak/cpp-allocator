## Decisions

Because of the vtable not being programmatically accessible, there is a problem in how to obtain an offset of an object based on a derived class pointer.
If it is not possible, we might have to choose between those two constraints (as we cannot determine the the slab position out of a raw pointer):
- not support base class pointer deletion (at least in case where the offset of that pointer is greater than the slab size)
- only support a single memory segment layout with a slab usage mask