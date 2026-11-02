#include "compiler/assets.h"

namespace dx::compiler {
    const std::vector<AssetHandler>& assetHandlers()
    {
        static const std::vector<AssetHandler> handlers = { textureAssets(), audioAssets(), meshAssets(), conversationAssets(), textAssets() };
        return handlers;
    }

    bool importAsset(ExecContext& context, const std::string& command)
    {
        for (const AssetHandler& handler : assetHandlers()) {
            if (handler.import && handler.import(context, command)) {
                return true;
            }
        }
        return false;
    }

    bool exportAssetSource(ExecContext& context, const std::string& command)
    {
        for (const AssetHandler& handler : assetHandlers()) {
            if (handler.exportSource && handler.exportSource(context, command)) {
                return true;
            }
        }
        return false;
    }

    bool exportAssetObject(UObject* object, const std::string& extension, const std::filesystem::path& file)
    {
        for (const AssetHandler& handler : assetHandlers()) {
            if (handler.exportObject && handler.exportObject(object, extension, file)) {
                return true;
            }
        }
        return false;
    }

    bool serializeAsset(PackageWriter& package, UObject* object, Writer& out)
    {
        for (const AssetHandler& handler : assetHandlers()) {
            if (handler.serialize && handler.serialize(package, object, out)) {
                return true;
            }
        }
        return false;
    }

    void finishAssets(ExecContext& context)
    {
        for (const AssetHandler& handler : assetHandlers()) {
            if (handler.finish) {
                handler.finish(context);
            }
        }
    }
}
