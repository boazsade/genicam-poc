from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps


class ImageServerRecipe(ConanFile):
    name = "DAArling"
    version = "1.0"
    package_type = "application"

    # Optional metadata
    license = "this is the sole property of GROWINGS ROBOTX LTD"
    author = "daa-team@growings.com"
    url = "www.growings.com"
    description = "Detect And Avoid on semi autonomous aircraft using the latest CV and AI technology"
    topics = ("AI", "CV", "Air Crafts")
    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    # Sources are located in the same place as this recipe, copy them to the recipe
    exports_sources = "CMakeLists.txt", "src/*"

    def layout(self):
        cmake_layout(self)

    def configure(self):
        self.options["opencv/*"].with_qt = False
        self.options["opencv/*"].with_gtk = True
        self.options["opencv/*"].with_wayland = False

    def requirements(self):
        self.requires("glog/0.7.0")
        self.requires("boost/1.85.0")
        if self.settings.os == "Windows":
            self.requires("opencv/4.9.0")
        else:
            self.requires("opencv/4.10.0")
        
    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.user_presets_path = 'ConanPresets.json'
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
