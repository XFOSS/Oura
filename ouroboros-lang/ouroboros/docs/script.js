document.addEventListener('DOMContentLoaded', () => {
    // --- Theme Toggle ---
    const themeBtn = document.getElementById('theme-btn');
    const body = document.body;
    const themeIcon = themeBtn.querySelector('i');

    const applyTheme = (theme) => {
        body.setAttribute('data-theme', theme);
        localStorage.setItem('theme', theme);
        if (theme === 'dark') {
            themeIcon.classList.remove('fa-moon');
            themeIcon.classList.add('fa-sun');
        } else {
            themeIcon.classList.remove('fa-sun');
            themeIcon.classList.add('fa-moon');
        }
    };

    const savedTheme = localStorage.getItem('theme') || (window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light');
    applyTheme(savedTheme);

    themeBtn.addEventListener('click', () => {
        const newTheme = body.getAttribute('data-theme') === 'light' ? 'dark' : 'light';
        applyTheme(newTheme);
    });

    // --- Smooth Scrolling & Active Navigation ---
    const navLinks = document.querySelectorAll('.nav-links a');
    const sections = document.querySelectorAll('section[id]'); // Ensure sections have IDs

    const observerOptions = {
        root: null, // relative to document viewport
        rootMargin: '0px 0px -70% 0px', // Adjust based on when you want active state to trigger
        threshold: 0.1 // visible amount of item shown in viewport
    };

    const activateNavLink = (id) => {
        navLinks.forEach(link => {
            link.classList.remove('active');
            if (link.getAttribute('href') === `#${id}`) {
                link.classList.add('active');
            }
        });
    };

    const sectionObserver = new IntersectionObserver((entries) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                activateNavLink(entry.target.id);
            }
        });
    }, observerOptions);

    sections.forEach(section => {
        sectionObserver.observe(section);
    });
    
    navLinks.forEach(link => {
        link.addEventListener('click', function(e) {
            e.preventDefault();
            const targetId = this.getAttribute('href');
            const targetElement = document.querySelector(targetId);
            if (targetElement) {
                // Calculate position considering potential fixed header/sidebar offset
                // This might need adjustment based on your actual fixed header height.
                const headerOffset = 60; // Example offset
                const elementPosition = targetElement.getBoundingClientRect().top + window.pageYOffset;
                const offsetPosition = elementPosition - headerOffset;
    
                window.scrollTo({
                    top: offsetPosition,
                    behavior: 'smooth'
                });
                // Manually update active state on click for immediate feedback
                // activateNavLink(targetId.substring(1));
                 // History update for better UX, if desired (optional)
                if (history.pushState) {
                    history.pushState(null, null, targetId);
                } else {
                    location.hash = targetId;
                }
            }
             // Close mobile sidebar on link click
            if (window.innerWidth <= 992 && sidebar.classList.contains('active')) {
                sidebar.classList.remove('active');
            }
        });
    });


    // --- Syntax Highlighting (Prism.js) ---
    // Ensure prism-ouroboros.js is loaded before this script or Prism.highlightAll will use defaults
    if (window.Prism) {
        Prism.highlightAll();
    }

    // --- Mobile Sidebar Toggle ---
    const sidebar = document.querySelector('.sidebar');
    const sidebarToggleBtn = document.createElement('button');
    sidebarToggleBtn.className = 'sidebar-toggle-btn';
    sidebarToggleBtn.innerHTML = '<i class="fas fa-bars"></i>';
    document.body.appendChild(sidebarToggleBtn);

    sidebarToggleBtn.addEventListener('click', () => {
        sidebar.classList.toggle('active');
        // Change icon on toggle
        if (sidebar.classList.contains('active')) {
            sidebarToggleBtn.innerHTML = '<i class="fas fa-times"></i>';
        } else {
            sidebarToggleBtn.innerHTML = '<i class="fas fa-bars"></i>';
        }
    });

    const handleResize = () => {
        if (window.innerWidth > 992) {
            sidebar.classList.remove('active'); // Ensure sidebar is not in 'active' mobile state
            sidebarToggleBtn.style.display = 'none';
            sidebar.style.transform = 'translateX(0)'; // Ensure sidebar is visible
        } else {
            sidebarToggleBtn.style.display = 'flex';
             if (!sidebar.classList.contains('active')) { // Only hide if not explicitly opened
                sidebar.style.transform = 'translateX(-100%)';
            }
        }
    };
    window.addEventListener('resize', handleResize);
    handleResize(); // Initial check


    // --- Copy Code Button Functionality ---
    document.querySelectorAll('pre').forEach(preBlock => {
        const copyButton = document.createElement('button');
        copyButton.className = 'copy-btn';
        copyButton.innerHTML = '<i class="fas fa-copy"></i> Copy';
        preBlock.appendChild(copyButton);

        copyButton.addEventListener('click', async () => {
            const codeToCopy = preBlock.querySelector('code').innerText;
            try {
                await navigator.clipboard.writeText(codeToCopy);
                copyButton.innerHTML = '<i class="fas fa-check"></i> Copied!';
                copyButton.classList.add('copied');
                setTimeout(() => {
                    copyButton.innerHTML = '<i class="fas fa-copy"></i> Copy';
                    copyButton.classList.remove('copied');
                }, 2000);
            } catch (err) {
                console.error('Failed to copy code: ', err);
                copyButton.innerText = 'Error';
                 setTimeout(() => {
                    copyButton.innerHTML = '<i class="fas fa-copy"></i> Copy';
                }, 2000);
            }
        });
    });

    // --- Fade-in Animations for Sections (More robust) ---
    const animatedSections = document.querySelectorAll('.section, .feature-card, .vs-feature, .function-doc');
    const fadeInObserver = new IntersectionObserver((entries, observerInstance) => {
        entries.forEach(entry => {
            if (entry.isIntersecting) {
                entry.target.classList.add('visible');
                observerInstance.unobserve(entry.target); // Stop observing once visible
            }
        });
    }, { threshold: 0.1 });

    animatedSections.forEach(el => {
        fadeInObserver.observe(el);
    });
    // Add this CSS for the .visible class (in styles.css or here for demo)
    // .section, .feature-card, etc. { opacity: 0; transform: translateY(20px); transition: opacity 0.6s ease-out, transform 0.6s ease-out; }
    // .visible { opacity: 1; transform: translateY(0); }

    // --- Keyboard Shortcuts (Example) ---
    document.addEventListener('keydown', (e) => {
        // Toggle theme with Ctrl/Cmd + Shift + L
        if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'L') {
            e.preventDefault();
            themeBtn.click();
        }
    });

});
