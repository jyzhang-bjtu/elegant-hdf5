#ifndef DATASPACE_H
#define DATASPACE_H

#include <vector>

#include <hdf5.h>

namespace h5cpp {

class Dataspace
{
public:
    Dataspace(hid_t dataspaceID);
    Dataspace(const Dataspace& other) = delete;
    Dataspace(const Dataspace &&other);
    Dataspace& operator=(const Dataspace &other) = delete;
    Dataspace& operator=(const Dataspace &&other);

    ~Dataspace();

    bool isValid() const;

    std::vector<hsize_t> extents() const;
    int dimensionCount() const;

    bool isScalar() const;
    bool isSimple() const;

    void close();

    friend class Attribute;
    friend class Dataset;
    friend class Object;
private:
    hid_t id() const;
    operator hid_t() const;
    H5S_class_t extentType() const;
    hid_t m_id;
};

} // namespace

#endif // DATASPACE_H