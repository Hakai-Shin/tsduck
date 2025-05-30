//----------------------------------------------------------------------------
//
// TSDuck - The MPEG Transport Stream Toolkit
// Copyright (c) 2005-2025, Thierry Lelegard
// BSD-2-Clause license, see LICENSE.txt file or https://tsduck.io/license
//
//----------------------------------------------------------------------------
//!
//!  @file
//!  Get information from GitHub about the releases of a project.
//!
//----------------------------------------------------------------------------

#pragma once
#include "tsReport.h"
#include "tsTime.h"
#include "tsUString.h"
#include "tsCerrReport.h"
#include "tsjsonValue.h"

namespace ts {

    class GitHubRelease;

    //!
    //! Smart pointer to a GitHubRelease (not thread-safe).
    //! @ingroup app
    //!
    using GitHubReleasePtr = std::shared_ptr<GitHubRelease>;

    //!
    //! Vector of smart pointers to GitHubRelease objects.
    //! @ingroup app
    //!
    using GitHubReleaseVector = std::vector<GitHubReleasePtr>;

    //!
    //! This class holds information from GitHub about a release of a project.
    //! @ingroup libtscore app
    //!
    class TSCOREDLL GitHubRelease
    {
    public:
        //!
        //! Default constructor.
        //!
        GitHubRelease() = default;

        //!
        //! Constructor with download of the version information from GitHub.
        //! Use isValid() to check if the download was successful.
        //! @param [in] owner Project owner, either a GitHub user name or organization name.
        //! @param [in] repository Project repository name.
        //! @param [in] tag Git tag of the version to fetch. If empty, fetch the latest version.
        //! @param [in,out] report Where to report error.
        //!
        GitHubRelease(const UString& owner, const UString& repository, const UString& tag = UString(), Report& report = CERR);

        //!
        //! Check if the content of this object is a valid release description.
        //! @return True if this object is valid.
        //!
        bool isValid() const { return _isValid; }

        //!
        //! Download the version information from GitHub.
        //! @param [in] owner Project owner, either a GitHub user name or organization name.
        //! @param [in] repository Project repository name.
        //! @param [in] tag Git tag of the version to fetch. If empty, fetch the latest version.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        bool downloadInfo(const UString& owner, const UString& repository, const UString& tag = UString(), Report& report = CERR);

        //!
        //! Description of an "asset" of the release (typically a binary installer).
        //!
        struct TSCOREDLL Asset
        {
            Asset() = default;          //!< Default constructor.
            UString        name {};            //!< File name (without URL or directory).
            std::uintmax_t size = 0;           //!< File size in bytes.
            UString        mimeType {};        //!< MIME type of the file content.
            UString        url {};             //!< URL to download the file.
            int            downloadCount = 0;  //!< Download count.
        };

        //!
        //! List of assets.
        //!
        using AssetList = std::list<Asset>;

        //!
        //! Get the Git tag name of the release.
        //! @return The Git tag name.
        //!
        UString tag() const;

        //!
        //! Get the version of the release.
        //! @return The version. This is the tag without leading letters, following
        //! the convention that versions "3.1" is tagged as "v3.1" for instance.
        //!
        UString version() const;

        //!
        //! Get the version name of the release.
        //! @return The version name.
        //!
        UString versionName() const;

        //!
        //! Get the publish date of the release.
        //! @return The publish date.
        //!
        Time publishDate() const;

        //!
        //! Get the URL of the source tarball (tgz file) for the release.
        //! @return The URL.
        //!
        UString sourceTarURL() const;

        //!
        //! Get the URL of the source zip archive for the release.
        //! @return The URL.
        //!
        UString sourceZipURL() const;

        //!
        //! Get the most appropriate URL of the source archive for the release.
        //! On Windows, prefer zip files. On UNIX, prefer tarballs.
        //! @return The URL.
        //!
        UString sourceURL() const;

        //!
        //! Get an appropriate file name to download sourceURL().
        //! @return The local file name.
        //! @see sourceURL()
        //!
        UString sourceFileName() const;

        //!
        //! Get the number of downloads for the assets of the release.
        //! @return The number of downloads for the assets.
        //!
        int assetDownloadCount() const;

        //!
        //! Get the list of all assets for the release.
        //! @param [out] assets The returned list of assets.
        //!
        void getAssets(AssetList& assets) const;

        //!
        //! Get the list of assets for the current platform.
        //! @param [out] assets The returned list of assets.
        //!
        void getPlatformAssets(AssetList& assets) const;

        //!
        //! Check if a binary file is appropriate for the current platform.
        //! The check is based on various naming conventions.
        //! @param [in] fileName Asset base file name (no directory).
        //! @return True is @a fileName seems appropriate for the local platform.
        //!
        static bool IsPlatformAsset(const UString& fileName);

        //!
        //! Download information from GitHub for all versions of a product.
        //! @param [out] versions Returned vector of downloaded versions.
        //! @param [in] owner Project owner, either a GitHub user name or organization name.
        //! @param [in] repository Project repository name.
        //! @param [in,out] report Where to report error.
        //! @return True on success, false on error.
        //!
        static bool GetAllVersions(GitHubReleaseVector& versions, const UString& owner, const UString& repository, Report& report = CERR);

    private:
        bool           _isValid = false;
        UString        _owner {};
        UString        _repository {};
        json::ValuePtr _root {};

        // Basic validation of the root JSON.
        bool validate(Report& report);

        // Check if we should use the source tarball or zip file.
        bool useSourceZip() const;

        // Fetch a API request for a repository. Return a JSON structure.
        static bool CallGitHub(json::ValuePtr& response, json::Type expectedType, const UString& owner, const UString& repository, const UString& request, Report& report);

        // Report an invalid response from GitHub.
        static void InvalidResponse(const json::ValuePtr& response, Report& report);

        // Build an asset description from a JSON object.
        static void BuildAsset(Asset& asset, const json::Value& value);

        // Convert a GitHub date/time string into a Time object.
        static Time StringToTime(const UString& str);
    };
}
