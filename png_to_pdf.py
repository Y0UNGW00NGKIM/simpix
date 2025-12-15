import argparse
import matplotlib

matplotlib.use("Agg")

import matplotlib.pyplot as plt
import matplotlib.image as mpimg


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input_png")
    parser.add_argument("output_pdf")
    parser.add_argument("--dpi", type=int, default=300)
    args = parser.parse_args()

    img = mpimg.imread(args.input_png)
    h = img.shape[0]
    w = img.shape[1]

    fig_w = w / args.dpi
    fig_h = h / args.dpi

    fig = plt.figure(figsize=(fig_w, fig_h), dpi=args.dpi)
    ax = fig.add_axes([0, 0, 1, 1])
    ax.axis("off")
    ax.imshow(img)
    fig.savefig(args.output_pdf, bbox_inches="tight", pad_inches=0)
    plt.close(fig)


if __name__ == "__main__":
    main()
