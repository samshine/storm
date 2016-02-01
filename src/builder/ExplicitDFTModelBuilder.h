#ifndef EXPLICITDFTMODELBUILDER_H
#define	EXPLICITDFTMODELBUILDER_H

#include "../storage/dft/DFT.h"

#include <src/models/sparse/StateLabeling.h>
#include <src/models/sparse/StandardRewardModel.h>
#include <src/models/sparse/Model.h>
#include <src/storage/SparseMatrix.h>
#include <src/storage/BitVectorHashMap.h>
#include <boost/container/flat_set.hpp>
#include <boost/optional/optional.hpp>
#include <stack>
#include <unordered_set>

namespace storm {
    namespace builder {

        template<typename ValueType, typename RewardModelType = storm::models::sparse::StandardRewardModel<ValueType>, typename IndexType = uint32_t>
        class ExplicitDFTModelBuilder {

            using DFTElementPointer = std::shared_ptr<storm::storage::DFTElement<ValueType>>;
            using DFTGatePointer = std::shared_ptr<storm::storage::DFTGate<ValueType>>;
            using DFTStatePointer = std::shared_ptr<storm::storage::DFTState<ValueType>>;

            // A structure holding the individual components of a model.
            struct ModelComponents {
                ModelComponents();

                // The transition matrix.
                storm::storage::SparseMatrix<ValueType> transitionMatrix;

                // The state labeling.
                storm::models::sparse::StateLabeling stateLabeling;

                // The reward models associated with the model.
                std::unordered_map<std::string, storm::models::sparse::StandardRewardModel<typename RewardModelType::ValueType>> rewardModels;

                // A vector that stores a labeling for each choice.
                boost::optional<std::vector<boost::container::flat_set<uint_fast64_t>>> choiceLabeling;
            };

            storm::storage::DFT<ValueType> const &mDft;
            storm::storage::BitVectorHashMap<DFTStatePointer> mStates;
            size_t newIndex = 0;

        public:
            ExplicitDFTModelBuilder(storm::storage::DFT<ValueType> const &dft) : mDft(dft), mStates(((mDft.stateSize() / 64) + 1) * 64, std::pow(2, mDft.nrBasicElements())) {
                // stateSize is bound for size of bitvector
                // 2^nrBE is upper bound for state space
            }

            std::shared_ptr<storm::models::sparse::Model<ValueType, RewardModelType>> buildCTMC();

        private:
            void exploreStates(std::queue<DFTStatePointer>& stateQueue, storm::storage::SparseMatrixBuilder<ValueType>& transitionMatrixBuilder);

        };
    }
}

#endif	/* EXPLICITDFTMODELBUILDER_H */