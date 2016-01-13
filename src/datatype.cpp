#include "datatype.h"

#include <stdexcept>

Datatype::Datatype()
{
}

Datatype::Datatype(hid_t typeID)
    : m_id(typeID)
{
    if(typeID < 1) {
        throw std::runtime_error("Invalid datatype");
    }
}

Datatype::Datatype(const Datatype &other)
{
    constructFromOther(other);
}

Datatype &Datatype::operator=(const Datatype &other)
{
    close();
    constructFromOther(other);
    return *this;
}

Datatype::~Datatype()
{
    close();
}

hid_t Datatype::id() const
{
    return m_id;
}

Datatype::operator hid_t() const
{
    return m_id;
}

bool Datatype::isValid() const
{
    if(m_id > 0) {
        return true;
    } else {
        return false;
    }
}

bool Datatype::isInt() const
{
    if(H5Tequal(m_id, H5T_NATIVE_INT)) {
        return true;
    } else {
        return false;
    }
}

bool Datatype::isFloat() const
{
    if(H5Tequal(m_id, H5T_NATIVE_FLOAT)) {
        return true;
    } else {
        return false;
    }
}

bool Datatype::isDouble() const
{
    if(H5Tequal(m_id, H5T_NATIVE_DOUBLE)) {
        return true;
    } else {
        return false;
    }
}

void Datatype::close()
{
    if(m_id > 0) {
        H5Tclose(m_id);
    }
}

void Datatype::constructFromOther(const Datatype &other) {
    if(other.isValid()) {
        m_id = H5Tcopy(other.id());
    } else {
        m_id = other.id();
    }
}
