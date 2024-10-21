## Installation

1. Download the `.tar.gz` file corresponding your os/architecture.
2. Run `tar -xvzf ./<your-file>.tar.gz`
3. Move `./jumper` to some a directory in your `$PATH`, e.g. `/usr/local/bin`. 
4. Add the following to your `.bashrc`, `.zshrc` or `.config/fish/config.fish` to get access to jumper's functions:
* bash
  ```sh
  eval "$(jumper shell bash)"
  ```
* zsh
  ```sh
  source <(jumper shell zsh)
  ```
* fish
  ```fish
  jumper shell fish | source
  ```
