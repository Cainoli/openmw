#ifndef OPENMW_MWWORLD_STORE_H
#define OPENMW_MWWORLD_STORE_H

#include <cctype>
#include <cassert>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace MWWorld
{
    std::string lowerCase(const std::string &in)
    {
        std::string out = in;
        std::transform(
            in.begin(),
            in.end(),
            out.begin(),
            (int (*)(int)) std::tolower
        );

        return out;
    }

    struct CICompareString
    {
        struct ci
        {
            bool operator()(int x, int y) {
                return std::tolower(x) < std::tolower(y);
            }
        };

        bool operator()(const std::string &x, const std::string &y) {
            return std::lexicographical_compare(
                x.begin(),
                x.end(),
                y.begin(),
                y.end(),
                ci()
            );
        }
    };

    struct StoreBase
    {
        class Compare
        {
            bool mCaseInsensitive;

        public:
            Compare()
              : mCaseInsensitive(false)
            {}

            Compare(bool ci)
              : mCaseInsensitive(ci)
            {}

            Compare(const Compare &orig)
              : mCaseInsensitive(orig.mCaseInsensitive)
            {}

            template<class T>
            bool operator()(const T &x, const T &y) {
                if (mCaseInsensitive) {
                    return CICompareString()(x.mId, y.mId);
                }
                return x.mId < y.mId;
            }

            template <>
            bool operator()<ESM::Cell>(const ESM::Cell &x, const ESM::Cell &y) {
                if (mCaseInsensitive) {
                    return CICompareString()(x.mName, y.mName);
                }
                return x.mName < y.mName;
            }

            bool isCaseInsensitive() const {
                return mCaseInsensitive;
            }

            bool equalString(const std::string &x, const std::string &y) const {
                if (!mCaseInsensitive) {
                    return x == y;
                }
                if (x.length() != y.length()) {
                    return false;
                }
                std::string::iterator xit = x.begin();
                std::string::iterator yit = y.begin();
                for (; xit != x.end(); ++xit, ++yit) {
                    if (std::tolower(*xit) != std::tolower(*yit)) {
                        return false;
                    }
                }
                return true;
            }
        };

        virtual ~StoreBase() {}

        virtual void setUp() {}
        virtual void listIdentifier(std::vector<std::string> &list) const {}

        virtual int getSize() const = 0;
        virtual void load(ESM::ESMReader &esm, const std::string &id) = 0;
    };

    template <class T>
    class SharedIterator
    {
        typedef typename std::vector<T *>::const_iterator Iter;

        Iter mIter;

    public:
        SharedIterator() {}

        SharedIterator(const SharedIterator &orig)
          : mIter(orig.mIter)
        {}

        SharedIterator(const Iter &iter)
          : mIter(iter)
        {}

        SharedIterator &operator++() {
            ++mIter;
            return *this;
        }

        SharedIterator operator++(int) {
            SharedIterator iter = *this;
            mIter++;

            return iter;
        }

        SharedIterator &operator--() {
            --mIter;
            return *this;
        }

        SharedIterator operator--(int) {
            SharedIterator iter = *this;
            mIter--;

            return iter;
        }

        SharedIterator operator==(const SharedIterator &x) const {
            return mIter == x.mIter;
        }

        SharedIterator operator!=(const SharedIterator &x) const {
            return !(*this == x);
        }

        const T &operator*() const {
            return **mIter;
        }

        const T *operator->() const {
            return &(**mIter);
        }
    };

    template <class T>
    class Store : public StoreBase
    {
        Compare             mComp;
        std::vector<T>      mData;
        std::vector<T *>    mShared;

    public:
        Store() {}

        Store(bool ci)
          : mComp(ci)
        {}

        Store(const Store<T> &orig)
          : mComp(orig.mComp), mData(orig.mData)
        {}

        typedef SharedIterator<T> iterator;

        const T* search(const std::string &id) const {
            T item;
            item.mId = lowerCase(id);

            typename std::vector<T>::const_iterator it =
                std::lower_bound(mData.begin(), mData.end(), item, mComp);

            if (it != mData.end() && mComp.equalString(it->mId, item.mId)) {
                return &(*it);
            }
            return 0;
        }

        const T *find(const std::string &id) const {
            const T *ptr = search(id);
            if (ptr == 0) {
                throw std::runtime_error("object '" + id + "' not found");
            }
            return ptr;
        }

        void setUp() {
            std::sort(mData.begin(), mData.end(), mComp);

            mShared.reserve(mData.size());
            typename std::vector<T>::iterator it = mData.begin();
            for (; it != mData.end(); ++it) {
                mShared.push_back(&(*it));
            }
        }

        iterator begin() {
            return mShared.begin();
        }

        iterator end() {
            return mShared.end();
        }

        int getSize() const {
            return mShared.size();
        }

        void listIdentifier(std::vector<std::string> &list) const {
            list.reserve(list.size() + getSize());
            typename std::vector<T *>::iterator it = mShared.begin();
            for (; it != mShared.end(); ++it) {
                list.push_back((*it)->mId);
            }
        }
    };

    template <>
    class Store<ESM::LandTexture> : public StoreBase
    {
        std::vector<ESM::LandTexture> mData;

    public:
        Store<ESM::LandTexture>() {
            mData.reserve(128);
        }

        typedef std::vector<ESM::LandTexture>::const_iterator iterator;

        const ESM::LandTexture *search(size_t index) const {
            if (index < mData.size()) {
                return &mData.at(index);
            }
            return 0;
        }

        const ESM::LandTexture *find(size_t index) const {
            ESM::LandTexture *ptr = search(index);
            if (ptr == 0) {
                throw std::runtime_error(
                    "Land texture with index " + index " not found"
                );
            }
            return ptr;
        }

        int getSize() const {
            return mData.size();
        }

        int load(ESM::ESMReader &esm, const std::string &id) {
            ESM::LandTexture ltex;
            ltex.load(esm);

            if (ltex.mIndex >= (int) mData.size()) {
                mData.resize(ltex.mIndex + 1);
            }
            mData[ltex.mIndex] = ltex;
            mData[ltex.mIndex].mId = id;
        }

        iterator begin() {
            return mData.begin();
        }

        iterator end() {
            return mData.end();
        }
    };

    template <>
    class Store<ESM::Land> : public StoreBase
    {
        std::vector<ESM::Land> mData;

        struct Compare
        {
            bool operator()(const ESM::Land &x, const ESM::Land &y) {
                if (x.mX == y.mX) {
                    return x.mY < y.mY;
                }
                return x.mX < y.mX;
            }
        };

    public:
        typedef typename std::vector<ESM::Land>::const_iterator iterator;

        int getSize() const {
            return mData.size();
        }

        iterator begin() {
            return mData.begin();
        }

        iterator end() {
            return mData.end();
        }

        ESM::Land *search(int x, int y) const {
            ESM::Land land;
            land.mX = x, land.mY = y;

            std::vector<ESM::Land>::iterator it =
                std::lower_bound(mData.begin(), mData.end(), land, Compare());

            if (it != mData.end() && it->mX == x && it->mY == y) {
                return &(*it);
            }
            return 0;
        }

        ESM::Land *find(int x, int y) const {
            ESM::Land *ptr = search(x, y);
            if (ptr == 0) {
                throw std::runtime_error(
                    "Land at (" + x + ", " + y + ") not found"
                );
            }
            return ptr;
        }

        void load(ESM::ESMReader &esm, const std::string &id) {
            mData.push_back(ESM::Land());
            mData.back().load(esm);
        }

        void setUp() {
            std::sort(mData.begin(), mData.end(), Compare());
        }
    };

    template <>
    class Store<ESM::Cell> : public StoreBase
    {
    public:
        typedef std::vector<ESM::Cell>::const_iterator iterator;

    private:
        struct ExtCompare
        {
            bool operator()(const ESM::Cell &x, const ESM::Cell &y) {
                if (x.mData.mX == y.mData.mX) {
                    return x.mData.mY < y.mData.mY;
                }
                return x.mData.mX < y.mData.mX;
            }
        };

        struct IntExtOrdering
        {
            bool operator()(const ESM::Cell &x, const ESM::Cell &y) {
                // interiors precedes exteriors (x < y)
                if ((x.mData.mFlags & ESM::Cell::Interior) != 0 &&
                    (y.mData.mFlags & ESM::Cell::Interior) == 0)
                {
                    return true;
                }
                return false;
            }
        };

        Compare                 mIntCmp;
        std::vector<ESM::Cell>  mData;
        iterator                mIntBegin, mIntEnd, mExtBegin, mExtEnd;

    public:
        Store<ESM::Cell>()
          : mIntCmp(true)
        {}


        const ESM::Cell *search(const std::string &id) const {
            ESM::Cell cell;
            cell.mName = id;

            iterator it =
                std::lower_bound(mIntBegin, mIntEnd, cell, mIntCmp);

            if (it != mIntEnd && mIntCmp.equalString(it->mId, id)) {
                return &(*it);
            }
            return 0;
        }

        const ESM::Cell *search(int x, int y) const {
            ESM::Cell cell;
            cell.mData.mX = x, cell.mData.mY = y;

            iterator it =
                std::lower_bound(mExtBegin, mExtEnd, cell, ExtCompare());

            if (it != mExtEnd && it->mData.mX == x && it->mData.mY == y) {
                return &(*it);
            }
            return 0;
        }

        const ESM::Cell *find(const std::string &id) const {
            ESM::Cell *ptr = search(id);
            if (ptr == 0) {
                throw std::runtime_error(
                    "Interior cell '" + id + "' not found"
                );
            }
            return ptr;
        }

        const ESM::Cell *find(int x, int y) const {
            ESM::Cell *ptr = search(x, y);
            if (ptr == 0) {
                throw std::runtime_error(
                    "Exterior cell at (" + x + ", " + y + ") not found"
                );
            }
            return ptr;
        }

        void setUp() {
            IntExtOrdering cmp;
            std::sort(mData.begin(), mData.end(), cmp);

            ESM::Cell cell;
            cell.mData.mFlags = 0;
            mExtBegin = std::lower_bound(mData.begin(), mData.end(), cell, cmp);
            mExtEnd = mData.end();

            mIntBegin = mData.begin();
            mIntEnd = mExtBegin;

            std::sort(mIntBegin, mIntEnd, mIntCmp);
            std::sort(mExtBegin, mExtEnd, ExtCompare());
        }

        void load(ESM::ESMReader &esm, const std::string &id) {
            mData.push_back(ESM::Cell());
            mData.back().mName = id;
            mData.back().load(esm);
        }

        iterator begin() {
            return mData.begin();
        }

        iterator end() {
            return mData.end();
        }

        iterator interiorsBegin() {
            return mIntBegin;
        }

        iterator interiorsEnd() {
            return mIntEnd;
        }

        iterator exteriorsBegin() {
            return mExtBegin;
        }

        iterator exteriorsEnd() {
            return mExtEnd;
        }

        /// \todo implement appropriate index
        const ESM::Cell *searchExteriorByName(const std::string &id) {
            for (iterator it = mExtBegin; it != mExtEnd; ++it) {
                if (mIntCmp.equalString(it->mName, id)) {
                    return &(*it);
                }
            }
            return 0;
        }

        /// \todo implement appropriate index
        const ESM::Cell *searchExteriorByRegion(const std::string &id) {
            for (iterator it = mExtBegin; it != mExtEnd; ++it) {
                if (mIntCmp.equalString(it->mRegion, id)) {
                    return &(*it);
                }
            }
            return 0;
        }

        int getSize() const {
            return mData.size();
        }

        void listIdentifier(std::vector<std::string> &list) const {
            list.reserve(list.size() + (mIntEnd - mIntBegin));
            std::vector<ESM::Cell>::iterator it = mIntBegin;
            for (; it != mIntEnd; ++it) {
                list.push_back(it->mName);
            }
        }
    };

    template <>
    class Store<ESM::Pathgrid> : public StoreBase
    {
    };

    template <>
    class Store<ESM::Script>
    {
    };

} //end namespace

#endif
