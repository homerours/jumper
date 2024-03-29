# Jumper's ranking algorithm

Jumper uses both frecency (frequency+recency at which items have been visited) and match accuracy (how well the query matches the path stored in the database).
The main idea is that, when only a few (say <=2) characters have been entered by the user (when fuzzy-finding a path), Jumper will mostly rank matches based on their frecency since these few characters contain very little information for picking the right match. However, as more characters are added, the user's prompt contains more information that can be used the improve the ranking: Jumper will put progressively more emphasis on match accuracy.

## Frecency
The frecency of a match measures the frequency and recency of the visits of the match. Assume that a match has been visited at times $T_0 > \cdots > T_n$, then at time $t$, we define
```math
\text{frecency}(t) = \log\left( \epsilon + 10 \, e^{- \alpha_1 (t - T_0)} + \sum_{i=0}^n e^{-\alpha_2 (t-T_i)} \right)
```
Here $\alpha_1 = 5 \times 10^{-5}$, $\alpha_2 = 3 \times 10^{-7}$ and all times are expressed in seconds. These values are chosen so that $e^{-\alpha_1 {\rm \ 4 \ hours}} \simeq 1 / 2$ and  $e^{-\alpha_2 {\rm \ 1\ month}} \simeq 1 / 2$. $\epsilon = 0.1$ enforces that the frecency remains bounded from below.

Let us now motivate a bit the definition of frecency above. 
Let us first consider an item that has not been visited within the last 10 hours, so that we can neglect the term $10 e^{- \alpha_1 (t - T_0)}$. 
Let's set $t=0$ as the origin of times.
Assume moreover that this item is typically visited every $T$ seconds, so that $T_i = - i T$ for $i=0,1,2, \dots$. Therefore
```math
\text{frecency}(t) 
\simeq \log\left( \sum_{i=0}^{\infty} e^{-\alpha_2 i T} \right)
 =  \log\left( \frac{1}{1 - e^{-\alpha_2 T}} \right)
```
We plot this function below:

![alt text](frecency.png)

In the case where the item has just been visited, the frecency above gets an increase of $+10$ inside of the $\log$, leading to the dashed curve. This allows directories that have been very recently visited but that do not have a long history of visits (think for instance at a newly created directory) to compete with older directories that have been visited for a very long time.

As we can see from the plot above, the frecency will typically be a number in the range $[0,6]$. Many other definitions for frecency are possible. We chose this one for the following reasons:
- It does not diverge at time goes. [z](https://github.com/rupa/z) uses something like `number-of-visits / time-since-last-visit`, which potentially diverges over time (and therefore require some "aging" mechanism).
- It only requires to keep track of the "adjusted" number of visits $\sum_i e^{-\alpha_2 (t-T_i)}$ and the time of last visit to be computed.

> [!NOTE]
> We presented above the frecency in the case where all visits had the same weight 1. However, Jumper can attribute different weights for different types of visit (using the `-w` option). Everytime a command is executed by the user, Jumper adds a visit of weight $w_i = 1$ to the current working directory (cwd) if the command has changed the cwd, and a visit of weight $w_i = 0.3$ otherwise.
> Then, the frecency is computed using the weighted sum $\sum_i w_i e^{-\alpha_2 (t-T_i)}$.


## Match accuracy

The match accuracy evaluates how well the query entered by the user matches the path stored in the database.
Similarly to the fuzzy-finders [fzf](https://github.com/junegunn/fzf) or [fzy](https://github.com/jhawthorn/fzy), this is done using a variant of the [Needleman-Wunsch algorith](https://en.wikipedia.org/wiki/Needleman–Wunsch_algorithm).

This finds the match that maximizes
```
U(match) = 10 * len(query) - 9 * (number-of-splits - 1) - total-length-of-gaps + bonuses(match)
```
The `bonuses` above give additional points if matches happen at special places, such at the end of the path, or beginning of words. Then the accuracy is

```math
\text{accuracy}(\text{query}, \text{path}) = \max_{\text{match}} U(\text{match}).
```
where the maximum is computed over all matches of `query` in `path`. We call "match" every (possibly non-contiguous) substring of `path` whose characters are the ones of `query`, in the same order (but with possibly with different case).

## Final score
Based on these two numbers, Jumper ranks paths using
```math
\text{score}(\text{query}, \text{path}, t) =  \text{frecency}(\text{path}, t) + \beta \, \text{accuracy}(\text{query}, \text{path}).
```
where $\beta = 0.5$ by default, and be updated with the flag `-b <value>`. 
This additive definition is motivated by the following.

Suppose that one is fuzzy-finding a path, adding one character to the `query` at a time.
At first, when `query` has very few character (typically <=2), all the paths containing these two characters consecutively will share the maximum `accuracy`.
Hence the ranking will be mostly decided by the frecency.
However, as more characters are added, the ranking will favors matches that are more accurate. The ranking will then be dominated by the accuracy of the matches.

## Statistical interpretation

The definitions of scores above can be motivated by the following statistical model. One can see the task of "finding the path the user thought about" given a query as a Bayesian inference problem:
- We assume that the paths picked by the user follow some prior distribution, which is based on the "frecency" of the paths.
- After picking the `path` he would like to jump to, the user is more likely to make a query that matches `path` with high accuracy. 
Hence the conditional distribution $P(\text{query}|\text{path})$ depends on $\text{accuracy}(\text{query}, \text{path})$.

Under this model, Jumper simply picks the path that maximizes the posterior probability $P(\text{path}|\text{query})$.
We give below more details about this Bayesian model.

**Prior distribution:** Assume that the visits of a given path is a self-exciting point process, with conditional intensity
```math
\lambda(t, \text{path}) = \epsilon +  10 e^{-\alpha_1 (t - T_0)} + \sum_{T_k \leq t} e^{-\alpha_2 (t - T_k)}
```
independently from the visits to the other folders.

When the user queries the database at a time $t$, he knows already the next folder he would like to visit, which is the folder whose point process has a jump at time $t$.
The user gave his query to the algorithm, which can be seen as a noisy observation of the path of the folder he would like to visit.

**Conditional distribution:** We model
```math
P(\text{query}|\text{path}) = \frac{1}{Z} \exp\Big(\beta \, \text{accuracy}(\text{query},\text{path})\Big)
```
($Z$ being here the appropriate normalizing constant) meaning that the user is more likely make query that have a large accuracy.

The posterior probability is therefore proportional to
```math
P(\text{path}|\text{query}) \propto \lambda(t, \text{path}) \exp\Big(\beta \,  \text{accuracy}(\text{query},\text{path})\Big)
```
The ranking algorithm simply ranks the paths according to their $\log$-posterior probability.
