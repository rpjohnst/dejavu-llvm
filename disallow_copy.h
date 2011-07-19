#ifndef DISALLOW_COPY_H
#define DISALLOW_COPY_H

#define DISALLOW_COPY(type) \
	type(const type&); \
	void operator =(const type&)

#endif
