
#ifndef DFT_H
#define	DFT_H


#include "DFTElements.h"
#include "../BitVector.h"
#include <memory>
#include <unordered_map>
#include <list>
#include <map>

#include "../../utility/math.h"
#include "src/utility/macros.h"
#include <boost/iterator/counting_iterator.hpp>

namespace storm {
    namespace storage {

        template<typename ValueType>
        struct DFTElementSort {
            bool operator()(std::shared_ptr<DFTElement<ValueType>> const& a, std::shared_ptr<DFTElement<ValueType>> const& b)  const {
                if (a->rank() == 0 && b->rank() == 0) {
                    return a->isConstant();
                } else {
                    return a->rank() < b->rank();
                }
            }
        };



        template<typename ValueType>
        class DFT {

            using DFTElementPointer = std::shared_ptr<DFTElement<ValueType>>;
            using DFTElementVector = std::vector<DFTElementPointer>;
            using DFTGatePointer = std::shared_ptr<DFTGate<ValueType>>;
            using DFTGateVector = std::vector<DFTGatePointer>;
            using DFTStatePointer = std::shared_ptr<DFTState<ValueType>>;

        private:
            DFTElementVector mElements;
            size_t mNrOfBEs;
            size_t mNrOfSpares;
            size_t mTopLevelIndex;
            size_t mUsageInfoBits;
            size_t mStateSize;
            std::map<size_t, size_t> mActivationIndex;
            std::map<size_t, std::vector<size_t>> mSpareModules;
            std::vector<size_t> mTopModule;
            std::vector<size_t> mIdToFailureIndex;
            std::map<size_t, size_t> mUsageIndex;
            std::map<size_t, size_t> mRepresentants;
            
        public:
            DFT(DFTElementVector const& elements, DFTElementPointer const& tle);

            size_t stateSize() const {
                return mStateSize;
            }
            
            size_t nrElements() const {
                return mElements.size();
            }
            
            size_t nrBasicElements() const {
                return mNrOfBEs;
            }
            
            size_t usageInfoBits() const {
                return mUsageInfoBits;
            }
            
            size_t usageIndex(size_t id) const {
                assert(mUsageIndex.find(id) != mUsageIndex.end());
                return mUsageIndex.find(id)->second;
            }
            
            size_t failureIndex(size_t id) const {
                return mIdToFailureIndex[id];
            }
            
            void initializeUses(DFTState<ValueType>& state) const {
                for(auto const& elem : mElements) {
                    if(elem->isSpareGate()) {
                        std::static_pointer_cast<DFTSpare<ValueType>>(elem)->initializeUses(state);
                    }
                }
            }
            
            void initializeActivation(DFTState<ValueType>& state) const {
                state.activate(mTopLevelIndex);
                for(auto const& elem : mTopModule) {
                    if(mElements[elem]->isSpareGate()) {
                        propagateActivation(state, state.uses(elem));
                    }
                }
            }
            
            std::vector<size_t> getSpareIndices() const {
                std::vector<size_t> indices;
                for(auto const& elem : mElements) {
                    if(elem->isSpareGate()) {
                        indices.push_back(elem->id());
                    }
                }
                return indices;
            }
            
            std::vector<size_t> const& module(size_t representativeId) const {
                if(representativeId == mTopLevelIndex) {
                    return mTopModule;
                } else {
                    assert(mSpareModules.count(representativeId)>0);
                    return mSpareModules.find(representativeId)->second;
                }
            }
            
            void propagateActivation(DFTState<ValueType>& state, size_t representativeId) const {
                state.activate(representativeId);
                for(size_t id : module(representativeId)) {
                    if(mElements[id]->isSpareGate()) {
                        propagateActivation(state, state.uses(id));
                    }
                }
            }
            
            std::vector<size_t> nonColdBEs() const {
                std::vector<size_t> result;
                for(DFTElementPointer elem : mElements) {
                    if(elem->isBasicElement() && !elem->isColdBasicElement()) {
                        result.push_back(elem->id());
                    }
                }
                return result;
            }

            DFTElementPointer const& getElement(size_t index) const {
                assert(index < nrElements());
                return mElements[index];
            }

            std::shared_ptr<DFTBE<ValueType>> getBasicElement(size_t index) const {
                assert(mElements[index]->isBasicElement());
                return std::static_pointer_cast<DFTBE<ValueType>>(mElements[index]);
            }

            std::vector<std::shared_ptr<DFTBE<ValueType>>> getBasicElements() const {
                std::vector<std::shared_ptr<DFTBE<ValueType>>> elements;
                for (DFTElementPointer elem : mElements) {
                    if (elem->isBasicElement()) {
                        elements.push_back(std::static_pointer_cast<DFTBE<ValueType>>(elem));
                    }
                }
                return elements;
            }

            bool hasRepresentant(size_t id) const {
                return mRepresentants.find(id) != mRepresentants.end();
            }

            DFTElementPointer getRepresentant(size_t id) const {
                assert(hasRepresentant(id));
                return getElement(mRepresentants.find(id)->second);
            }

            bool hasFailed(DFTStatePointer const& state) const {
                return state->hasFailed(mTopLevelIndex);
            }
            
            bool isFailsafe(DFTStatePointer const& state) const {
                return state->isFailsafe(mTopLevelIndex);
            }
            
            std::string getElementsString() const;

            std::string getInfoString() const;

            std::string getSpareModulesString() const;

            std::string getElementsWithStateString(DFTStatePointer const& state) const;

            std::string getStateString(DFTStatePointer const& state) const;
            
        private:
            bool elementIndicesCorrect() const {
                for(size_t i = 0; i < mElements.size(); ++i) {
                    if(mElements[i]->id() != i) return false;
                }
                return true;
            }

        };
       
    }
}
#endif	/* DFT_H */

