#ifndef __BOA_POINTERANALYZER_H
#define __BOA_POINTERANALYZER_H

namespace boa {

class PointerAnalyzer {
 private:
  set<Buffer> buffers_;
  
 public:
  void SetBuffers(const set<Buffer> &buffers) {
    buffers_ = buffers;
  }
  
  const set<Buffer>& PointsTo(const Pointer&) {
    return buffers_;
  }
};

}  // namespace boa

#endif  // __BOA_POINTERANALYZER_H
