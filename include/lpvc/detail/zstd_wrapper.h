#ifndef LIBLPVC_DETAIL_ZSTD_WRAPPER_H
#define LIBLPVC_DETAIL_ZSTD_WRAPPER_H

#include <memory>
#include <zstd.h>


namespace lpvc
{


// ===========================================================================
//  ZSTDCCtxDeleter
// ===========================================================================

struct ZSTDCCtxDeleter final
{
  void operator()(ZSTD_CCtx* ctx) const noexcept
  {
    ZSTD_freeCCtx(ctx);
  }
};

using ZSTDCCtx = std::unique_ptr<ZSTD_CCtx, ZSTDCCtxDeleter>;


// ===========================================================================
//  ZSTDDCtxDeleter
// ===========================================================================

struct ZSTDDCtxDeleter final
{
  void operator()(ZSTD_DCtx* ctx) const noexcept
  {
    ZSTD_freeDCtx(ctx);
  }
};

using ZSTDDCtx = std::unique_ptr<ZSTD_DCtx, ZSTDDCtxDeleter>;


} // namespace lpvc


#endif // LIBLPVC_DETAIL_ZSTD_WRAPPER_H
