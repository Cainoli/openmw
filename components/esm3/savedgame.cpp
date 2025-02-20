#include "savedgame.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include "../misc/algorithm.hpp"

namespace ESM
{
    void SavedGame::load(ESMReader& esm)
    {
        mPlayerName = esm.getHNString("PLNA");
        esm.getHNOT(mPlayerLevel, "PLLE");

        mPlayerClassId = esm.getHNORefId("PLCL");
        mPlayerClassName = esm.getHNOString("PLCN");

        if (esm.getFormatVersion() <= ESM::MaxSavedGameCellNameAsRefIdFormatVersion)
            mPlayerCellName = esm.getHNRefId("PLCE").toString();
        else
            mPlayerCellName = esm.getHNString("PLCE");
        esm.getHNT("TSTM", mInGameTime.mGameHour, mInGameTime.mDay, mInGameTime.mMonth, mInGameTime.mYear);
        esm.getHNT(mTimePlayed, "TIME");
        mDescription = esm.getHNString("DESC");

        while (esm.isNextSub("DEPE"))
            mContentFiles.push_back(esm.getHString());

        esm.getSubNameIs("SCRN");
        esm.getSubHeader();
        mScreenshot.resize(esm.getSubSize());
        esm.getExact(mScreenshot.data(), mScreenshot.size());

        esm.getHNOT(mCurrentDay, "CDAY");
        esm.getHNOT(mCurrentHealth, "CHLT");
        esm.getHNOT(mMaximumHealth, "MHLT");
    }

    void SavedGame::save(ESMWriter& esm) const
    {
        esm.writeHNString("PLNA", mPlayerName);
        esm.writeHNT("PLLE", mPlayerLevel);

        if (!mPlayerClassId.empty())
            esm.writeHNRefId("PLCL", mPlayerClassId);
        else
            esm.writeHNString("PLCN", mPlayerClassName);

        esm.writeHNString("PLCE", mPlayerCellName);
        esm.writeHNT("TSTM", mInGameTime, 16);
        esm.writeHNT("TIME", mTimePlayed);
        esm.writeHNString("DESC", mDescription);

        for (std::vector<std::string>::const_iterator iter(mContentFiles.begin()); iter != mContentFiles.end(); ++iter)
            esm.writeHNString("DEPE", *iter);

        esm.startSubRecord("SCRN");
        esm.write(mScreenshot.data(), mScreenshot.size());
        esm.endRecord("SCRN");

        esm.writeHNT("CDAY", mCurrentDay);
        esm.writeHNT("CHLT", mCurrentHealth);
        esm.writeHNT("MHLT", mMaximumHealth);
    }

    std::vector<std::string_view> SavedGame::getMissingContentFiles(
        const std::vector<std::string>& allContentFiles) const
    {
        std::vector<std::string_view> missingFiles;
        for (const std::string& contentFile : mContentFiles)
        {
            auto it = std::find_if(allContentFiles.begin(), allContentFiles.end(),
                [&](const std::string& file) { return Misc::StringUtils::ciEqual(file, contentFile); });
            if (it == allContentFiles.end())
            {
                missingFiles.emplace_back(contentFile);
            }
        }

        return missingFiles;
    }
}
