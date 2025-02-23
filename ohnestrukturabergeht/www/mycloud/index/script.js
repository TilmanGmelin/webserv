document.addEventListener("DOMContentLoaded", () => {
    const menuToggle = document.querySelector(".menu-toggle");
    const sidebar = document.querySelector(".sidebar");

    menuToggle.addEventListener("click", () => {
        sidebar.classList.toggle("collapsed");
    });

    const darkModeToggle = document.querySelector(".dark-mode-toggle");

    darkModeToggle.addEventListener("click", () => {
        document.body.classList.toggle("dark-mode");
    });

    const ctx = document.getElementById('chart').getContext('2d');
    new Chart(ctx, {
        type: 'line',
        data: {
            labels: ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun'],
            datasets: [{
                label: 'Revenue ($)',
                data: [1200, 1500, 1800, 2000, 2500, 3000],
                borderColor: '#007bff',
                backgroundColor: 'rgba(0, 123, 255, 0.2)',
                borderWidth: 2
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false
        }
    });
});


document.addEventListener("DOMContentLoaded", function () {
    const uploadArea = document.getElementById("upload-area");
    const fileInput = document.getElementById("file-input");
    const fileList = document.getElementById("file-list");

    uploadArea.addEventListener("click", () => fileInput.click());
    fileInput.addEventListener("change", handleFiles);
    
    uploadArea.addEventListener("dragover", (e) => {
        e.preventDefault();
        uploadArea.style.background = "rgba(213, 241, 177, 0.57)";
		uploadArea.style.backgroundImage = "url('https://www.pngplay.com/wp-content/uploads/8/Upload-Icon-Logo-Transparent-File.png')";
		uploadArea.style.backgroundRepeat = "no-repeat";
		uploadArea.style.backgroundSize = "280px";
		uploadArea.style.backgroundPosition = "center";
    });

    uploadArea.addEventListener("dragleave", () => {
        uploadArea.style.background = "transparent";
		uploadArea.style.background = "white";
		uploadArea.style.backgroundImage = "url('https://www.pngplay.com/wp-content/uploads/8/Upload-Icon-Logo-Transparent-File.png')";
		uploadArea.style.backgroundRepeat = "no-repeat";
		uploadArea.style.backgroundSize = "300px";
		uploadArea.style.backgroundPosition = "center";
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
		document.getElementById("file-list").style.display = "block";
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

