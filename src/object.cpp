#include "object.h"

#include "dataset.h"
#include "group.h"
#include "io/typehelper.h"

using namespace std;

namespace elegant {
namespace hdf5 {

Object::Object(ConversionFlags conversionFlags)
    : m_inheritedConversionFlags(conversionFlags)
{
}

Object::Object(hid_t id, hid_t parentID, string name, ConversionFlags inheritedConversionFlags)
    : m_id(id)
    , m_parentId(parentID)
    , m_name(name)
    , m_inheritedConversionFlags(inheritedConversionFlags)
{
}

Object::Object(const Object &other, Object::CopyMode mode)
    : m_parentId(other.parentID())
    , m_name(other.name())
{
    if(mode == CopyMode::OpenOnCopy) {
        constructFromOther(other);
    }
}

//Object::Object(Object &&other)
//    : m_id(move(other.m_id))
//    , m_parentID(move(other.m_parentID))
//    , m_name(move(other.m_name))
//{
//#ifdef H5CPP_VERBOSE
//    DVLOG(1) << "Move constructor object " << m_id;
//#endif
//    other.m_id = 0;
//}

//Object& Object::operator=(Object &&other)
//{
//    if(m_id > 0 && other.id() > 0) {
//        if(type() == Type::Dataset && other.type() == Type::Dataset) {
//            DVLOG(1) << "We could copy, but it's not yet implemented";
//        } else {

//        }
//    }
//#ifdef H5CPP_VERBOSE
//    DVLOG(1) << "Move assignment of object " << other.m_id << " (overwriting " << m_id << ")";
//#endif
//    swap(m_id, other.m_id); // Swap to make sure our id is cleaned up by other
//    m_parentID = move(other.m_parentID);
//    m_name = move(other.m_name);
//    return *this;
//}

Object &Object::operator=(const Object &other)
{
    bool copyFromExistingToExisting = isValid() && other.isValid();
    bool copyFromExistingToNonExisting = isNonExistingNamed() && other.isValid();

    bool isSame = (m_name == other.name() && m_parentId == other.parentID());
    if(isSame) {
        DVLOG(1) << "Is the same object";
        return *this;
    } else if(copyFromExistingToExisting || copyFromExistingToNonExisting) {
        close();
        if(copyFromExistingToExisting) {
            H5Ldelete(m_parentId, m_name.c_str(), H5P_DEFAULT);
        }
        herr_t error = H5Ocopy(other.parentID(), other.name().c_str(),
                m_parentId, m_name.c_str(), H5P_DEFAULT, H5P_DEFAULT);
        if(error < 0) {
            throw runtime_error("Could not copy object from other");
        }
        m_id = H5Oopen(m_parentId, m_name.c_str(), H5P_DEFAULT);
        if(m_id < 1) {
            throw runtime_error("Could not open object");
        }
        return *this;
    }
    constructFromOther(other);
    return *this;
}

Object &Object::operator =(const Dataset &other)
{
    const Object& otherObject = other;
    Object::operator =(otherObject);
    return *this;
}

Object &Object::operator =(const Group &other)
{
    const Object& otherObject = other;
    Object::operator =(otherObject);
    return *this;
}

void Object::constructFromOther(const Object &other)
{
    close();
    if(other.isValid()) {
        m_id = H5Oopen(other.id(), ".", H5P_DEFAULT);
        if(m_id < 1) {
            throw runtime_error("Could not open object from other");
        }
        DVLOG(1) << "Opened other object " << other << " to " << m_id;
    } else {
        m_id = other.id();
        DVLOG(1) << "Copied other " << m_id;
    }
    m_name = other.m_name;
    m_parentId = other.m_parentId;
    m_inheritedConversionFlags = other.m_inheritedConversionFlags;
}

void Object::close()
{
    if(m_id > 0) {
        DVLOG(1) << "Closing object " << m_id;
        herr_t error = H5Oclose(m_id);
        if(error < 0) {
            DVLOG(1) << "WARNING: Could not safely close object " << *this;
        }
        m_id = 0;
    }
}

Object::~Object()
{
    close();
}

const std::string& Object::name() const
{
    return m_name;
}

Object::Type Object::type() const
{
    if(m_id < 1) {
        return Type::Invalid;
    }
    H5I_type_t hType = H5Iget_type(m_id);
    return Object::fromHdf5Type(hType);
}

hid_t Object::id() const
{
    return m_id;
}

bool Object::isValid() const
{
    if(type() == Type::File) {
        if(m_id == 0) {
            return false;
        }
        return true;
    }
    if(m_id == 0 || m_name.empty() || m_parentId == 0) {
        return false;
    }
    return true;
}

bool Object::isDataset() const
{
    return (isValid() && type() == Type::Dataset);
}

bool Object::isGroup() const
{
    return (isValid() && type() == Type::Group);
}

bool Object::isNonExistingNamed() const
{
    return (!isValid() && !m_name.empty() && m_parentId > 0);
}

Object::Type Object::fromHdf5Type(H5I_type_t hType) {
    Type returnedType = Type::Invalid;
    switch(hType) {
    case H5I_FILE:
        returnedType = Type::File;
        break;
    case H5I_GROUP:
        returnedType = Type::Group;
        break;
    case H5I_DATATYPE:
        returnedType = Type::Datatype;
        break;
    case H5I_DATASPACE:
        returnedType = Type::Dataspace;
        break;
    case H5I_DATASET:
        returnedType = Type::Dataset;
        break;
    case H5I_ATTR:
        returnedType = Type::Attribute;
        break;
    default:
        break;
    }
    return returnedType;
}

H5I_type_t Object::toHdf5Type(Object::Type hType) {
    H5I_type_t returnedType = H5I_BADID;
    switch(hType) {
    case Type::File:
        returnedType = H5I_FILE;
        break;
    case Type::Group:
        returnedType = H5I_GROUP;
        break;
    case Type::Datatype:
        returnedType = H5I_DATATYPE;
        break;
    case Type::Dataspace:
        returnedType = H5I_DATASPACE;
        break;
    case Type::Dataset:
        returnedType = H5I_DATASET;
        break;
    case Type::Attribute:
        returnedType = H5I_ATTR;
        break;
    default:
        break;
    }
    return returnedType;
}

hid_t Object::parentID() const
{
    return m_parentId;
}

vector<string> Object::attributeKeys() const
{
    vector<string> returnedAttributes;
    hsize_t idx = 0;
    H5_index_t l = H5_INDEX_NAME; // TODO: Move to function param?
    H5Aiterate(m_id, l, H5_ITER_INC, &idx,
               [&](hid_t g_id, const char *name, const H5A_info_t *info, void *namesVoid) -> herr_t {
        (void)g_id;
        (void)info;
        vector<string> &names = *(vector<string>*)(namesVoid);

        string nameString(name);
        names.push_back(nameString);

        return 0;
    }, &returnedAttributes);

    return returnedAttributes;
}

Attribute Object::operator()(string key) const
{
    return attribute(key);
}

Attribute Object::attribute(string key) const
{
    if(!isValid()) {
        throw std::runtime_error("Trying to access attribute of invalid object");
    }
    if(!hasAttribute(key)) {
        return Attribute(0, m_id, key, m_inheritedConversionFlags);
    }
    return Attribute(m_id, key, m_inheritedConversionFlags);
}

vector<Attribute> Object::attributes() const
{
    if(!isValid()) {
        throw std::runtime_error("Trying to access attributes of invalid object");
    }
    vector<Attribute> returnedAttributes;
    for(string key : attributeKeys()) {
        returnedAttributes.emplace_back(attribute(key));
    }
    return returnedAttributes;
}

bool Object::hasAttribute(string name) const
{
    if(!isValid()) {
        throw std::runtime_error("Trying to probe attribute of invalid object");
    }
    if(H5Aexists(m_id, name.c_str()) != true) {
        return false;
    }
    return true;
}


}
}
