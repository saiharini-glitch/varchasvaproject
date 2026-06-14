#pragma once
#include <cstdarg>
namespace Eloquent {
    namespace ML {
        namespace Port {
            class RandomForest {
                public:
                    /**
                    * Predict class for features vector
                    */
                    int predict(float *x) {
                        uint8_t votes[1] = { 0 };
                        // tree #1
                        votes[0] += 1;
                        // tree #2
                        votes[0] += 1;
                        // tree #3
                        votes[0] += 1;
                        // tree #4
                        votes[0] += 1;
                        // tree #5
                        votes[0] += 1;
                        // tree #6
                        votes[0] += 1;
                        // tree #7
                        votes[0] += 1;
                        // tree #8
                        votes[0] += 1;
                        // tree #9
                        votes[0] += 1;
                        // tree #10
                        votes[0] += 1;
                        // return argmax of votes
                        uint8_t classIdx = 0;
                        float maxVotes = votes[0];

                        for (uint8_t i = 1; i < 1; i++) {
                            if (votes[i] > maxVotes) {
                                classIdx = i;
                                maxVotes = votes[i];
                            }
                        }

                        return classIdx;
                    }

                protected:
                };
            }
        }
    }