#ifndef DATASET_H
#define DATASET_H

#include "object.h"
#include "typehelper.h"
#include "logging.h"
#include "demangle.h"

#include <iostream>
#include <typeinfo>

namespace h5cpp {

class Dataset : public Object
{
public:
    Dataset();
    Dataset(hid_t id, hid_t parentID, std::string name);

    Dataset(const Object &other);
    Dataset(const Dataset &other);
    //    Dataset(Dataset &&other);

    ~Dataset();

    Dataset& operator=(const Object &other);
    Dataset& operator=(const Dataset &other);

    template<typename T>
    Dataset& operator=(const T &data);

    std::vector<hsize_t> extents() const;

    template<typename T>
    static Dataset create(hid_t parentID, const std::string &name, const T &data);

    template<typename T>
    operator T();
private:
    std::vector<hsize_t> extents(hid_t dataspace) const;
};

template<>
inline Object::operator Dataset() {
    return Dataset(*this);
}

template<typename T>
inline void Object::operator=(const T& matrix)
{
    DVLOG(1) << "Assignment operator of T";
    DVLOG(1) << "Is valid: " << isValid();
    if(isValid()) {
        Dataset dataset = *this;
        dataset = matrix;
    } else if(m_parentID > 0) {
        Dataset::create(m_parentID, m_name, matrix);
    }
}

template<typename T>
Dataset& Dataset::operator=(const T &data)
{
    DVLOG(1) << "Dataset assignment operator of T type: " << typeid(T).name();
    DVLOG(1) << "Parent, name, id: " << m_parentID << " " << m_name << " " << m_id;
    if(m_id == 0 && m_parentID > 0) {
        *this = Dataset::create(m_parentID, m_name, data);
    } else {
        if(!isValid()) {
            throw(std::runtime_error("Assigning value to invalid dataset object"));
            return *this;
        }
        int targetDimensions = TypeHelper<T>::dimensionCount();
        hid_t dataspace = H5Dget_space(m_id);
        if(dataspace < 1) {
            throw std::runtime_error("Could not get dataspace");
            return *this;
        }
        std::vector<hsize_t> extent = extents(dataspace);
        int currentDimensions = extent.size();
        bool shouldOverwrite = false;
        if(currentDimensions != targetDimensions || !TypeHelper<T>::matchingExtents(data, extent)) {
            shouldOverwrite = true;
        }

        if(shouldOverwrite) {
            DVLOG(1) << "WARNING: Writing over dataset of different shape. "
                     << "Limitations in HDF5 standard makes it impossible to free space taken "
                     << "up by the old dataset.";
            herr_t closeError = H5Sclose(dataspace);
            if(closeError < 0) {
                throw std::runtime_error("Could not close dataspace");
            }
            close();
            herr_t deleteError = H5Ldelete(m_parentID, m_name.c_str(), H5P_DEFAULT);
            if(deleteError < 0) {
                throw std::runtime_error("Could not delete old dataset");
            }
            *this = Dataset::create(m_parentID, m_name, data);
        } else {
            DVLOG(1) << "Writing to old dataset";
            hid_t datatype = TypeHelper<T>::hdfType();
            TypeHelper<T> temporary;
            herr_t errors = H5Dwrite(m_id, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, temporary.readBuffer(data));
            H5Sclose(dataspace);
            if(errors < 0) {
                DVLOG(1) << "Error writing to dataset!";
            }
        }
    }
    return *this;
}

template<typename T>
Dataset Dataset::create(hid_t parentID, const std::string &name, const T &data)
{
    DVLOG(1) << "Creating dataset on parent " << parentID << " with name " << name;
    int targetDimensions = TypeHelper<T>::dimensionCount();
    std::vector<hsize_t> extents = TypeHelper<T>::extentsFromType(data);
    DVLOG(1) << "Extents: " << extents[0] << " "
               << (targetDimensions > 1 ? extents[1] : 0) << " "
               << (targetDimensions > 2 ? extents[2] : 0);

    hid_t dataspace = H5Screate_simple(targetDimensions, &extents[0], NULL);
    hid_t creationParameters = H5Pcreate(H5P_DATASET_CREATE);
    hid_t datatype = TypeHelper<T>::hdfType();
    hid_t dataset = H5Dcreate(parentID, name.c_str(), datatype, dataspace,
                              H5P_DEFAULT, creationParameters, H5P_DEFAULT);
    if(dataset > 0) {
        TypeHelper<T> temporary;
        herr_t errors = H5Dwrite(dataset, datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, temporary.readBuffer(data));
        if(errors >= 0) {
            H5Sclose(dataspace);
            DVLOG(1) << "Returning the created dataset " << dataset;
            return Dataset(dataset, parentID, name);
        }
    };
    return Dataset(0, 0, name);
}

template<typename T>
inline Dataset::operator T()
{
    if(!isValid()) {
        throw(std::runtime_error("Could not fetch value from invalid dataset object"));
    }
    DVLOG(1) << "Getting dataspace for " << m_id;
    std::vector<hsize_t> extent = extents();
    int dimensionCount = extent.size();
    if(dimensionCount != TypeHelper<T>::dimensionCount()) {
        std::stringstream errorStream;
        errorStream << "Tried to copy dataspace with " << dimensionCount
                    << " dimensions to type " << demangle(typeid(T).name());
        throw std::runtime_error(errorStream.str());
    }

    T object = TypeHelper<T>::objectFromExtents(extent);

    hid_t hdf5Datatype = TypeHelper<T>::hdfType();
    herr_t readError = H5Dread(m_id, hdf5Datatype, H5S_ALL, H5S_ALL, H5P_DEFAULT, TypeHelper<T>::writeBuffer(object));
    if(readError < 0) {
        throw std::runtime_error("Could not read dataset");
    }
    return object;
}

}

#endif // DATASET_H
