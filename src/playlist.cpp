/*
 * This file is part of Bino, a 3D video player.
 *
 * Copyright (C) 2022
 * Martin Lambers <marlam@marlam.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QFile>
#include <QTextStream>
#include <QCommandLineParser>

#include "playlist.hpp"
#include "log.hpp"


PlaylistEntry::PlaylistEntry() :
    url(), inputMode(Input_Unknown), threeSixtyMode(ThreeSixty_Unknown),
    videoTrack(NoTrack), audioTrack(NoTrack), subtitleTrack(NoTrack)
{
}

PlaylistEntry::PlaylistEntry(const QUrl& url,
        InputMode inputMode,
        ThreeSixtyMode threeSixtyMode,
        int videoTrack, int audioTrack, int subtitleTrack) :
    url(url), inputMode(inputMode), threeSixtyMode(threeSixtyMode),
    videoTrack(videoTrack), audioTrack(audioTrack), subtitleTrack(subtitleTrack)
{
}

bool PlaylistEntry::noMedia() const
{
    return url.isEmpty();
}

QString PlaylistEntry::optionsToString() const
{
    QString s;
    if (inputMode != Input_Unknown) {
        s.append(" --input=");
        s.append(inputModeToString(inputMode));
    }
    if (threeSixtyMode != ThreeSixty_Unknown) {
        s.append(" --360=");
        s.append(threeSixtyModeToString(threeSixtyMode));
    }
    if (videoTrack != PlaylistEntry::DefaultTrack) {
        s.append(" --video-track=");
        s.append(QString::number(videoTrack));
    }
    if (audioTrack != PlaylistEntry::DefaultTrack) {
        s.append(" --audio-track=");
        s.append(QString::number(audioTrack));
    }
    if (subtitleTrack != PlaylistEntry::NoTrack) {
        s.append(" --subtitle-track=");
        s.append(QString::number(subtitleTrack));
    }
    return s;
}

bool PlaylistEntry::optionsFromString(const QString& s)
{
    QCommandLineParser parser;
    parser.addOption({ "input", "", "x" });
    parser.addOption({ "360", "", "x" });
    parser.addOption({ "video-track", "", "x" });
    parser.addOption({ "audio-track", "", "x" });
    parser.addOption({ "subtitle-track", "", "x" });
    if (!parser.parse((QString("dummy ") + s).split(' ')) || parser.positionalArguments().length() != 0) {
        return false;
    }
    InputMode inputMode = Input_Unknown;
    ThreeSixtyMode threeSixtyMode = ThreeSixty_Unknown;
    int videoTrack = PlaylistEntry::DefaultTrack;
    int audioTrack = PlaylistEntry::DefaultTrack;
    int subtitleTrack = PlaylistEntry::NoTrack;
    bool ok = true;
    if (parser.isSet("input")) {
        inputMode = inputModeFromString(parser.value("input"), &ok);
    }
    if (parser.isSet("360")) {
        threeSixtyMode = threeSixtyModeFromString(parser.value("360"), &ok);
    }
    if (parser.isSet("video-track")) {
        int t = parser.value("video-track").toInt(&ok);
        if (ok && t >= 0)
            videoTrack = t;
        else
            ok = false;
    }
    if (parser.isSet("audio-track")) {
        int t = parser.value("audio-track").toInt(&ok);
        if (ok && t >= 0)
            audioTrack = t;
        else
            ok = false;
    }
    if (parser.isSet("subtitle-track") && parser.value("subtitle-track").length() > 0) {
        int t = parser.value("subtitle-track").toInt(&ok);
        if (ok && t >= 0)
            subtitleTrack = t;
        else
            ok = false;
    }
    if (ok) {
        *this = PlaylistEntry(this->url, inputMode, threeSixtyMode, videoTrack, audioTrack, subtitleTrack);
    }
    return ok;
}


static Playlist* playlistSingleton = nullptr;

Playlist::Playlist() :
    _preferredAudio(QLocale::system().language()),
    _preferredSubtitle(QLocale::system().language()),
    _wantSubtitle(false),
    _loopMode(Loop_Off),
    _currentIndex(-1)
{
    Q_ASSERT(!playlistSingleton);
    playlistSingleton = this;
}

Playlist* Playlist::instance()
{
    return playlistSingleton;
}

QLocale::Language Playlist::preferredAudio() const
{
    return _preferredAudio;
}

void Playlist::setPreferredAudio(const QLocale::Language& lang)
{
    _preferredAudio = lang;
}

QLocale::Language Playlist::preferredSubtitle() const
{
    return _preferredSubtitle;
}

void Playlist::setPreferredSubtitle(const QLocale::Language& lang)
{
    _preferredSubtitle = lang;
}

bool Playlist::wantSubtitle() const
{
    return _wantSubtitle;
}

void Playlist::setWantSubtitle(bool want)
{
    _wantSubtitle = want;
}

const QList<PlaylistEntry>& Playlist::entries() const
{
    return _entries;
}

void Playlist::emitMediaChanged()
{
    if (_currentIndex >= 0 && _currentIndex < length()) {
        emit mediaChanged(_entries[_currentIndex]);
    } else {
        emit mediaChanged(PlaylistEntry());
    }
}

int Playlist::length() const
{
    return _entries.length();
}

void Playlist::append(const PlaylistEntry& entry)
{
    _entries.append(entry);
}

void Playlist::insert(int index, const PlaylistEntry& entry)
{
    _entries.insert(index, entry);
}

void Playlist::remove(int index)
{
    if (index >= 0 && index < length()) {
        _entries.remove(index);
        if (_currentIndex == index) {
            if (_currentIndex >= length())
                _currentIndex--;
            emitMediaChanged();
        } else if (_currentIndex > index) {
            _currentIndex--;
        }
    }
}

void Playlist::clear()
{
    _entries.clear();
    int prevIndex = _currentIndex;
    _currentIndex = -1;
    if (prevIndex >= 0)
        emitMediaChanged();
}

void Playlist::start()
{
    if (length() > 0 && _currentIndex < 0)
        setCurrentIndex(0);
}

void Playlist::stop()
{
    setCurrentIndex(-1);
}

void Playlist::next()
{
    if (length() > 0) {
        if (_currentIndex == length() - 1)
            setCurrentIndex(0);
        else
            setCurrentIndex(_currentIndex++);
    }
}

void Playlist::prev()
{
    if (length() > 0) {
        if (_currentIndex == 0)
            setCurrentIndex(length() - 1);
        else
            setCurrentIndex(_currentIndex--);
    }
}

void Playlist::setCurrentIndex(int index)
{
    if (index == _currentIndex) {
        return;
    }

    if (length() == 0 || index < 0) {
        _currentIndex = -1;
        emitMediaChanged();
        return;
    }

    if (index >= length()) {
        index = length() - 1;
    }
    if (_currentIndex != index) {
        _currentIndex = index;
        emitMediaChanged();
    }
}

LoopMode Playlist::loopMode() const
{
    return _loopMode;
}

void Playlist::setLoopMode(LoopMode loopMode)
{
    _loopMode = loopMode;
}

void Playlist::mediaEnded()
{
    if (loopMode() == Loop_One) {
        // keep current index but play again
        emitMediaChanged();
    } else if (_currentIndex == length() - 1 && loopMode() == Loop_All) {
        // start with first index
        setCurrentIndex(0);
    } else if (_currentIndex < length() - 1) {
        // start with next index
        setCurrentIndex(_currentIndex + 1);
    }
}

bool Playlist::save(const QString& fileName, QString& errStr) const
{
    QFile file(fileName);
    if (!file.open(QIODeviceBase::WriteOnly | QIODeviceBase::Truncate | QIODeviceBase::Text)) {
        errStr = file.errorString();
        return false;
    }

    QTextStream out(&file);
    out << "#EXTM3U\n";
    for (int i = 0; i < length(); i++) {
        out << "#EXTINF:0,\n";
        out << "#EXTBINOOPT:" << entries()[i].optionsToString() << "\n";
        out << entries()[i].url.toString() << "\n";
    }
    file.flush();
    file.close();
    if (file.error() != QFileDevice::NoError) {
        errStr = file.errorString();
        return false;
    }
    return true;
}


bool Playlist::load(const QString& fileName, QString& errStr)
{
    QFile file(fileName);
    if (!file.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text)) {
        errStr = file.errorString();
        return false;
    }

    QList<PlaylistEntry> entries;
    PlaylistEntry entry;
    QTextStream in(&file);
    int lineIndex = 0;
    while (!in.atEnd()) {
        QString line = in.readLine();
        lineIndex++;
        if (line.startsWith("#EXTBINOOPT: ")) {
            if (!entry.optionsFromString(line.mid(13))) {
                LOG_DEBUG("%s line %d: ignoring invalid Bino options", qPrintable(fileName), lineIndex);
            }
        } else if (line.isEmpty() || line.startsWith("#")) {
            continue;
        } else {
            entry.url = QUrl::fromUserInput(line, QString("."), QUrl::AssumeLocalFile);
            if (entry.url.isValid() && (
                        entry.url.scheme() == "file"
                        || entry.url.scheme() == "https"
                        || entry.url.scheme() == "http")) {
                entries.append(entry);
            } else {
                LOG_DEBUG("%s line %d: ignoring invalid URL", qPrintable(fileName), lineIndex);
            }
        }
    }
    if (file.error() == QFileDevice::NoError) {
        // overwrite this playlist with new information
        _entries = entries;
        _currentIndex = -1;
        return true;
    } else {
        errStr = file.errorString();
        return false;
    }
}
