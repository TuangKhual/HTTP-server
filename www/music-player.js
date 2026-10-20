const songImage = document.getElementById("image");
const songName = document.getElementById("song");
const songArtist = document.getElementById("song-artist");

const songSlider = document.getElementById("slider-song");

const playButton = document.getElementById("play");
const pauseButton = document.getElementById("pause");
const backButton = document.getElementById("back");
const nextButton = document.getElementById("next");

const songs = [
    {
        image: "./Invincible.png",
        name: "Invincible",
        artist: "DEAF KEV",
        audio: "./Invincible.mp3"
    },
    {
        image: "./Blank.png",
        name: "Blank",
        artist: "Disfigure",
        audio: "./Blank.mp3"
    },
    {
        image: "./Heroes.png",
        name: "Heroes",
        artist: "Janji",
        audio: "./Heroes.mp3"
    },
];

const audio = document.createElement("audio");
let currentSongIndex = 0;

updateSong();

backButton.addEventListener("click", function() {
    if (currentSongIndex == 0){
        return;
    }
    currentSongIndex--;
    updateSong();
})

nextButton.addEventListener("click", function() {
    if (currentSongIndex == songs.length - 1){
        return;
    }
    currentSongIndex++;
    updateSong();
})

playButton.addEventListener("click", function() {
    audio.play();
})

pauseButton.addEventListener("click", function(){
    audio.pause();
})

audio.addEventListener("play", function() {
    playButton.style.display = "none";
    pauseButton.style.display = "inline-block";
    songImage.classList.add("playing");
})

audio.addEventListener("pause", function() {
    pauseButton.style.display = "none";
    playButton.style.display = "inline-block";
    songImage.classList.remove("playing");
})

function updateSong() {
    const wasPlaying = !audio.paused;

    const song = songs[currentSongIndex];
    songImage.src = song.image;
    songName.innerText = song.name;
    songArtist.innerText = song.artist;

    audio.src = song.audio;
    songSlider.value = 0;
    audio.onloadedmetadata = function() {
        songSlider.max = audio.duration;
    }

    if (wasPlaying){
        audio.play();
    }
}

songSlider.addEventListener("change", function() {
    audio.currentTime = songSlider.value;
})

function moveSlider(){
    songSlider.value = audio.currentTime;
};

setInterval(moveSlider, 1000)