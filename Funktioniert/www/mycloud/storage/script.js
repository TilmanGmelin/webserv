document.addEventListener("DOMContentLoaded", function () {
    const uploadArea = document.getElementById("upload-area");
    const fileInput = document.getElementById("file-input");
    const fileList = document.getElementById("file-list");

    uploadArea.addEventListener("click", () => fileInput.click());
    fileInput.addEventListener("change", handleFiles);
    
    uploadArea.addEventListener("dragover", (e) => {
        e.preventDefault();
        uploadArea.style.background = "rgba(255, 255, 255, 0.3)";
    });

    uploadArea.addEventListener("dragleave", () => {
        uploadArea.style.background = "transparent";
    });

    uploadArea.addEventListener("drop", (e) => {
        e.preventDefault();
        uploadArea.style.background = "transparent";
        handleFiles(e.dataTransfer);
    });

    function handleFiles(event) {
        const files = event.files || event.target.files;
        for (let file of files) {
            addFile(file);
        }
    }

    function addFile(file) {
        const listItem = document.createElement("li");
        listItem.classList.add("file-item");
        listItem.innerHTML = `
            ${file.name}
            <button class="delete-btn">X</button>
        `;

        fileList.appendChild(listItem);

        listItem.querySelector(".delete-btn").addEventListener("click", () => {
            listItem.remove();
        });
    }
});
